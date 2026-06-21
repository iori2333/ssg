///
/// MIDI output via TinySoundFont + miniaudio
///

#include "audio/midi_backend.h"

#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#define TSF_IMPLEMENTATION
#include <miniaudio.h>
#include <tsf.h>

#include "config.h"
#include "audio/midi.h"
#include "sys/path.h"

static tsf *TsSoundFont = nullptr;
static ma_device TsDevice;
static bool TsAudioRunning = false;

static std::vector<std::string> TsSf2Paths;
static size_t TsSf2Index = 0;

static constexpr int SAMPLE_RATE = 44100;

static void AudioCallback(ma_device *, void *pOutput, const void *,
                          ma_uint32 frameCount) {
  if (TsSoundFont) {
    tsf_render_float(TsSoundFont, static_cast<float *>(pOutput), frameCount,
                     false);
  }
}

static std::string_view Basename(std::string_view path) {
  const auto sep = path.find_last_of("/\\");
  return (sep == std::string::npos) ? path : path.substr(sep + 1);
}

static void ScanSoundFonts(std::string_view data_path) {
  TsSf2Paths.clear();
  const std::string sf_dir =
      std::string{data_path.data(), data_path.size()} + "soundfonts";

  std::error_code ec;
  if (!std::filesystem::is_directory(sf_dir, ec)) {
    return;
  }

  for (const auto &entry : std::filesystem::directory_iterator(sf_dir, ec)) {
    if (entry.is_regular_file() && entry.path().extension() == ".sf2") {
      TsSf2Paths.push_back(entry.path().string());
    }
  }

  std::ranges::sort(TsSf2Paths);
}

static size_t FindSoundFont(std::string_view name) {
  if (name.empty()) {
    return SIZE_MAX;
  }
  for (size_t i = 0; i < TsSf2Paths.size(); i++) {
    if (Basename(TsSf2Paths[i]) == name) {
      return i;
    }
  }
  return SIZE_MAX;
}

static bool TsInitAudio(void) {
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate = SAMPLE_RATE;
  config.dataCallback = AudioCallback;

  if (ma_device_init(nullptr, &config, &TsDevice) != MA_SUCCESS) {
    return false;
  }

  ma_device_start(&TsDevice);
  TsAudioRunning = true;
  return true;
}

static void TsCleanupAudio(void) {
  if (TsAudioRunning) {
    ma_device_stop(&TsDevice);
    TsAudioRunning = false;
  }
  ma_device_uninit(&TsDevice);
}

static void TsSaveCurrent(void) {
  if (TsSf2Index < TsSf2Paths.size()) {
    ConfigDat.soundfont = Basename(TsSf2Paths[TsSf2Index]);
  }
}

bool MidBackend_Init(void) {
  if (TsSoundFont) {
    return true;
  }

  ScanSoundFonts(PathForData());
  if (TsSf2Paths.empty()) {
    return false;
  }

  TsSf2Index = FindSoundFont(ConfigDat.soundfont);
  if (TsSf2Index == SIZE_MAX) {
    TsSf2Index = 0;
    TsSaveCurrent();
  }

  TsSoundFont = tsf_load_filename(TsSf2Paths[TsSf2Index].c_str());
  if (!TsSoundFont) {
    return false;
  }

  tsf_set_output(TsSoundFont, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);

  if (!TsInitAudio()) {
    tsf_close(TsSoundFont);
    TsSoundFont = nullptr;
    return false;
  }

  return true;
}

void MidBackend_Cleanup(void) {
  TsCleanupAudio();
  if (TsSoundFont) {
    tsf_close(TsSoundFont);
    TsSoundFont = nullptr;
  }
}

std::optional<std::string_view> MidBackend_DeviceName(void) {
  if (!TsSoundFont || TsSf2Index >= TsSf2Paths.size()) {
    return std::nullopt;
  }
  const auto name = Basename(TsSf2Paths[TsSf2Index]);
  static thread_local std::string cached;
  cached = name;
  return cached;
}

bool MidBackend_DeviceChange(int8_t direction) {
  if (TsSf2Paths.size() <= 1) {
    return true; // No other SoundFont to switch to, but don't stop playback
  }

  const auto old_index = TsSf2Index;
  if (direction > 0) {
    TsSf2Index = (TsSf2Index + 1) % TsSf2Paths.size();
  } else {
    TsSf2Index = (TsSf2Index + TsSf2Paths.size() - 1) % TsSf2Paths.size();
  }

  TsCleanupAudio();
  tsf_close(TsSoundFont);
  TsSoundFont = nullptr;

  TsSoundFont = tsf_load_filename(TsSf2Paths[TsSf2Index].c_str());
  if (!TsSoundFont) {
    TsSf2Index = old_index;
    TsSoundFont = tsf_load_filename(TsSf2Paths[TsSf2Index].c_str());
  }

  tsf_set_output(TsSoundFont, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);

  if (!TsInitAudio()) {
    TsCleanupAudio();
    tsf_close(TsSoundFont);
    TsSoundFont = nullptr;
    TsSf2Index = old_index;
    TsSoundFont = tsf_load_filename(TsSf2Paths[TsSf2Index].c_str());
    tsf_set_output(TsSoundFont, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
    TsInitAudio();
    return false;
  }

  TsSaveCurrent();
  return true;
}

static SDL_TimerID TsTimer = 0;

static constexpr auto TIMER_INTERVAL = std::chrono::milliseconds(10);

extern "C" uint32_t TimerCallback(void *, SDL_TimerID, uint32_t interval) {
  Mid_Proc(std::chrono::duration_cast<MID_REALTIME>(
      std::chrono::milliseconds{interval}));
  return (uint32_t)TIMER_INTERVAL.count();
}

void MidBackend_StartTimer(void) {
  if (!TsTimer) {
    TsTimer = SDL_AddTimer(TIMER_INTERVAL.count(), TimerCallback, nullptr);
  }
}

void MidBackend_StopTimer(void) {
  if (TsTimer) {
    SDL_RemoveTimer(TsTimer);
    TsTimer = 0;
  }
}

void MidBackend_Out(uint8_t status, uint8_t a, uint8_t b) {
  if (!TsSoundFont) {
    return;
  }

  const int ch = (status & 0x0F);

  switch (status & 0xF0) {
  case 0x80:
    tsf_channel_note_off(TsSoundFont, ch, a);
    break;

  case 0x90:
    if (b == 0) {
      tsf_channel_note_off(TsSoundFont, ch, a);
    } else {
      tsf_channel_note_on(TsSoundFont, ch, a, b / 127.0f);
    }
    break;

  case 0xB0:
    tsf_channel_midi_control(TsSoundFont, ch, a, b);
    break;

  case 0xC0:
    tsf_channel_set_presetnumber(TsSoundFont, ch, a, (ch == 9));
    break;

  case 0xE0: {
    int val = (a | (b << 7));
    tsf_channel_set_pitchwheel(TsSoundFont, ch, val);
    break;
  }
  }
}

void MidBackend_Out(std::span<uint8_t> event) {
  if (event.size() >= 1) {
    uint8_t a = (event.size() >= 2) ? event[1] : 0;
    uint8_t b = (event.size() >= 3) ? event[2] : 0;
    MidBackend_Out(event[0], a, b);
  }
}

void MidBackend_Panic(void) {
  if (!TsSoundFont) {
    return;
  }
  for (int ch = 0; ch < 16; ch++) {
    tsf_channel_sounds_off_all(TsSoundFont, ch);
  }
}
