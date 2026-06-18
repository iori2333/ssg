///
/// MIDI backend implementation via WinMM / MME / midiOut*()
///

#include "game/midi.h"
#include "platform/midi_backend.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <mmeapi.h>

#pragma comment(lib, "winmm.lib")

using namespace std::chrono_literals;

using DEVICE_NAME = std::array<char, MAXPNAMELEN>;

typedef struct {
  // MIDI structure
  HMIDIOUT mp;          // Output device handle
  unsigned int nDevice; // Number of devices
  int NowID;            // Current device ID
  DEVICE_NAME *name;    // Device names

  // Timer structure
  UINT htimer;
  TIMECAPS caps;
  std::chrono::duration<uint32_t, std::milli> delay;
} MID_WINMM;

// Global, namespace-local variable
static MID_WINMM Mid_WinMM;

bool MidBackend_Init(void) {
  // Initialize variables
  Mid_WinMM.mp = nullptr;
  Mid_WinMM.nDevice = (midiOutGetNumDevs() + 1); // Allocate one extra
  Mid_WinMM.NowID = (-1 + 1);                    // -1: MIDI_MAPPER
  Mid_WinMM.name = nullptr;

  // Return if no devices exist
  constexpr auto device_cap =
      ((std::numeric_limits<size_t>::max)() / sizeof(DEVICE_NAME));
  if ((Mid_WinMM.nDevice > 0) && (Mid_WinMM.nDevice < device_cap)) {
    Mid_WinMM.name = static_cast<DEVICE_NAME *>(
        LocalAlloc(LPTR, (sizeof(DEVICE_NAME) * Mid_WinMM.nDevice)));
  }
  if (!Mid_WinMM.name) {
    return false;
  }

  // Allocate and populate device name buffer
  for (decltype(Mid_WinMM.nDevice) i = 0; i < Mid_WinMM.nDevice; i++) {
    MIDIOUTCAPS caps;
    midiOutGetDevCaps((i - 1), &caps, sizeof(MIDIOUTCAPS));
    std::ranges::copy(caps.szPname, Mid_WinMM.name[i].begin());
  }

  // Find a usable device (MIDI_MAPPER is first — that's the trick)
  for (decltype(Mid_WinMM.nDevice) i = 0; i < Mid_WinMM.nDevice; i++) {
    Mid_WinMM.NowID = i;
    const auto mret =
        midiOutOpen(&Mid_WinMM.mp, (Mid_WinMM.NowID - 1), 0, 0, CALLBACK_NULL);
    if (mret == MMSYSERR_NOERROR) {
      return true;
    }
  }

  // No usable devices found
  MidBackend_Cleanup();
  return FALSE;
}

void MidBackend_Cleanup(void) {
  if (Mid_WinMM.mp) {
    midiOutClose(Mid_WinMM.mp);
    Mid_WinMM.mp = nullptr;
  }

  if (Mid_WinMM.name) {
    LocalFree(Mid_WinMM.name);
    Mid_WinMM.name = nullptr;
  }

  Mid_WinMM.nDevice = Mid_WinMM.NowID = 0;
}

std::optional<std::string_view> MidBackend_DeviceName(void) {
  if (Mid_WinMM.mp == nullptr) {
    return std::nullopt;
  }
  return {Mid_WinMM.name[Mid_WinMM.NowID].data()};
}

bool MidBackend_DeviceChange(int8_t direction) {
  midiOutClose(Mid_WinMM.mp);

  const auto temp =
      (Mid_WinMM.nDevice + Mid_WinMM.NowID + ((direction < 0) ? -1 : 1));
  for (decltype(Mid_WinMM.nDevice) i = 0; i < Mid_WinMM.nDevice; i++) {
    Mid_WinMM.NowID =
        (((direction < 0) ? (temp - i) : (temp + i)) % Mid_WinMM.nDevice);
    const auto mret =
        midiOutOpen(&Mid_WinMM.mp, (Mid_WinMM.NowID - 1), 0, 0, CALLBACK_NULL);
    if (mret == MMSYSERR_NOERROR) {
      return true;
    }
  }

  // This shouldn't happen, but...
  return false;
}

static void CALLBACK CBMid_TimeFunc(UINT uID, UINT uMsg, DWORD_PTR dwUser,
                                    DWORD_PTR dw1, DWORD_PTR dw2) {
  Mid_Proc(Mid_WinMM.delay);
}

void MidBackend_StartTimer(void) {
  timeGetDevCaps(&Mid_WinMM.caps, sizeof(TIMECAPS));
  timeBeginPeriod(Mid_WinMM.caps.wPeriodMin);
  Mid_WinMM.delay = 10ms;
  Mid_WinMM.htimer =
      timeSetEvent(Mid_WinMM.delay.count(), Mid_WinMM.caps.wPeriodMin,
                   CBMid_TimeFunc, 0, TIME_PERIODIC);
}

void MidBackend_StopTimer(void) {
  timeKillEvent(Mid_WinMM.htimer);
  timeEndPeriod(Mid_WinMM.caps.wPeriodMin);
}

void MidBackend_Out(uint8_t byte_1, uint8_t byte_2, uint8_t byte_3) {
  const DWORD data = ((byte_1 << 0) | (byte_2 << 8) | (byte_3 << 16));
  midiOutShortMsg(Mid_WinMM.mp, data);
}

void MidBackend_Out(std::span<uint8_t> event) {
  MIDIHDR mh = {
      .lpData = reinterpret_cast<LPSTR>(event.data()),
      .dwBufferLength = static_cast<DWORD>(event.size_bytes()),
      .dwBytesRecorded = static_cast<DWORD>(event.size_bytes()),
  };
  midiOutPrepareHeader(Mid_WinMM.mp, &mh, sizeof(MIDIHDR));
  midiOutLongMsg(Mid_WinMM.mp, &mh, sizeof(MIDIHDR));
  midiOutUnprepareHeader(Mid_WinMM.mp, &mh, sizeof(MIDIHDR));
}

void MidBackend_Panic(void) { midiOutReset(Mid_WinMM.mp); }
