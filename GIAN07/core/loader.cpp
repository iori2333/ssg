///
/// Loader - Resource loading and MIDI loop point references
///

#include <cassert>
#include <cstring>
#include <utility>

#include "config.h"
#include "lz_uty.h"

#include "audio/midi.h"
#include "audio/snd.h"
#include "core/gian.h"
#include "enemy/enemy.h"
#include "gfx/format_bmp.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "stage/music.h"
#include "stage/window_sys.h"
#include "sys/path.h"
#include "util/enum_array.h"

#include "scripts_data.h"

static BYTE_BUFFER_BORROWED LoadEmbeddedScript(int filno) {
  for (size_t i = 0; i < embedded_script_count; i++) {
    if (embedded_scripts[i].index == filno) {
      return BYTE_BUFFER_BORROWED(embedded_scripts[i].data,
                                  embedded_scripts[i].size);
    }
  }
  return {};
}

// Packfile loading //
bool GrpBMPLoadP(const PACKFILE_READ &in, fil_no_t filno, SURFACE_ID sid) {
  auto maybe_bmp = BMPLoad(in.MemExpand(filno));

  // If this fails, we're going to crash due to the uninitialized surface
  // anyway. Might as well announce it in debug mode.
  assert(maybe_bmp);

  // Still necessary to avoid generation of exception handling code in
  // Release mode.
  if (!maybe_bmp) {
    return false;
  }

  auto &bmp = maybe_bmp.value();
  return GrpSurface_Load(sid, std::move(bmp));
}

bool Snd_SELoadP(const PACKFILE_READ &in, fil_no_t filno, uint8_t id, int max) {
  return Snd_SELoad(in.MemExpand(filno), id, max);
}

bool LoadSound(const PACKFILE_READ &in);

// Packfile cache //
// -------------- //

namespace DAT {
enum class PACK_ID : uint8_t {
  MAP,
  IMAGES,
  MUSIC,
  SOUND,
  COUNT,
};

constexpr ENUMARRAY<std::string_view, PACK_ID> BASENAMES = {{
    "MAP.PAK",
    "IMAGES.PAK",
    "MUSIC.PAK",
    "SOUND.PAK",
}};

constexpr std::string_view NOT_FOUND = "\xe2\x98\x90 ";
constexpr std::string_view FOUND = "\xe2\x98\x91 ";

// Packfiles load synchronously at startup. A default-constructed
// `PACKFILE_READ` is uninitialized; after a successful Load(), `pack`
// holds the parsed data ready for extraction.
class PACK {
private:
  PACKFILE_READ pack;
  std::string filename_with_found_prefix;

public:
  bool Load(std::string_view path_data, PACK_ID id);
  [[nodiscard]] const std::string &FilenameWithFoundPrefix() const {
    return filename_with_found_prefix;
  }

  const PACKFILE_READ &Get() const { return pack; }
};
ENUMARRAY<PACK, PACK_ID> Packs;

// For MUSIC.PAK, cache track metadata during loading
struct MusicMeta {
  std::string title;   // UTF-8
  std::string comment; // UTF-8, \n-separated
};
std::vector<MusicMeta> MusicMetas;

const PACKFILE_READ &Packfile(PACK_ID id) {
  return Packs[id].Get();
}

void LoadMusicMetadata(const PACKFILE_READ &in) {
  MusicNum = in.info.size();
  MusicMetas.resize(MusicNum);

  for (auto i = 0; std::cmp_less(i, MusicNum); i++) {
    if (const auto file = in.MemExpand(i)) {
      auto cursor = file.cursor();

      std::string_view title, comment;
      if (const auto title_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
        const auto title_len = title_len_val.value()[0];
        if (cursor.cursor + title_len <= cursor.size()) {
          title = {reinterpret_cast<const char *>(&cursor[cursor.cursor]),
                   title_len};
          cursor.next<uint8_t>(title_len);
        }
      }
      if (const auto comment_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
        const auto comment_len = comment_len_val.value()[0];
        if (cursor.cursor + comment_len <= cursor.size()) {
          comment = {reinterpret_cast<const char *>(&cursor[cursor.cursor]),
                     comment_len};
          cursor.next<uint8_t>(comment_len);
        }
      }

      MusicMetas[i].title = title;
      MusicMetas[i].comment = comment;
    } else {
      assert(!"Failure extracting BGM file?");
    }
  }
};

bool PACK::Load(std::string_view path_data, PACK_ID id) {
  if (pack) {
    return true;
  }
  if (filename_with_found_prefix.empty()) {
    static_assert(NOT_FOUND.size() == FOUND.size());
    const auto basename = BASENAMES[id];
    const auto cap = (NOT_FOUND.size() + path_data.size() + basename.size());
    filename_with_found_prefix.resize_and_overwrite(
        cap, [&](char *buf, size_t) {
          std::ranges::in_out_result p = {.in = path_data.begin(), .out = buf};
          p = std::ranges::copy(NOT_FOUND, p.out);
          p = std::ranges::copy(path_data, p.out);
          p = std::ranges::copy(basename, p.out);
          return (p.out - buf);
        });
  }
  auto *stream = SDL_IOFromFile(
      (filename_with_found_prefix.c_str() + NOT_FOUND.size()), "rb");
  if (stream == nullptr) {
    return false;
  }
  std::ranges::copy(FOUND, filename_with_found_prefix.begin());
  auto in = FilStartR(stream);
  if (id == PACK_ID::MUSIC) {
    LoadMusicMetadata(in);
  } else if (id == PACK_ID::SOUND) {
    LoadSound(in);
  }
  pack = std::move(in);
  return true;
}

bool Check() {
  const auto path_data = PathForData();
  bool ret = true;
  for (const auto i : std::views::iota(0U, BASENAMES.size())) {
    const auto id = Cast::down_enum<DAT::PACK_ID>(i);
    ret &= Packs[id].Load(path_data, id);
  }
  return ret;
}

// Load the n-th song //
bool LoadMusic(fil_no_t filno) {
  const auto &music = Packs[PACK_ID::MUSIC].Get();
  if (filno >= static_cast<int>(MusicMetas.size())) {
    return false;
  }

  auto raw = music.MemExpand(filno);
  if (!raw) {
    return false;
  }

  auto cursor = raw.cursor();
  if (const auto title_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
    const auto title_len = title_len_val.value()[0];
    cursor.next<uint8_t>(title_len);
  }
  if (const auto comment_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
    const auto comment_len = comment_len_val.value()[0];
    cursor.next<uint8_t>(comment_len);
  }

  const auto midi_size = raw.size() - cursor.cursor;
  BYTE_BUFFER_OWNED midi_buf(midi_size);
  if (midi_buf) {
    std::memcpy(midi_buf.get(), raw.get() + cursor.cursor, midi_size);
  }

  return Mid_Load(std::move(midi_buf));
}

bool LoadMusicByIndex(int index) {
  Packs[PACK_ID::MUSIC].Get();
  if ((index < 0) || (index >= static_cast<int>(MusicMetas.size()))) {
    return false;
  }
  return LoadMusic(index);
}
} // namespace DAT
// -------------- //

// Missing packfile screen
// -----------------------

namespace DAT_MISSING {
constexpr std::string_view TITLE = "Missing game data files";

bool FoundAll = false;

bool FnRecheck(MenuController & /*ctrl*/, INPUT_BITS key) {
  if ((key == KEY_BOMB) || (key == KEY_ESC)) {
    return false;
  }
  if ((Input_OptionKeyDelta(key) != 0) && DAT::Check()) {
    FoundAll = true;
    return false;
  }
  return true;
}

constexpr auto CENTER = MenuFlags::CENTER;
MenuLabel Title = {TITLE.data(), CENTER};
std::array<MenuItem, (DAT::BASENAMES.size() + 6)> Info = {{
    {},
    {},
    {},
    {},
    {},
    {},
    {"Must be provided from an original game copy.", "", CENTER},
    {},
    {"Recheck", "", FnRecheck, CENTER},
    {"Quit", "", CWinExitFn, CENTER},
}};
MenuDef Menu = {std::span(Info), [](MenuController &, bool) {}, &Title};
MenuController Window(Menu);

void Proc(bool &quit) {
  Window.Tick(Key_Data);
  if (!Window.Active()) {
    if (FoundAll) {
      SProjectInit();
    } else {
      quit = true;
    }
  }
  if (GameFlow.IsDraw()) {
    GrpBackend_Clear();
    Window.Draw();
    Grp_Flip();
  }
}

void Init() {
  for (const auto i : std::views::iota(0U, DAT::BASENAMES.size())) {
    const auto id = Cast::down_enum<DAT::PACK_ID>(i);
    const auto &title = DAT::Packs[id].FilenameWithFoundPrefix();
    Info[1 + i].Title = title.c_str();
  }
  const auto w = (std::max)(CWinTextExtent(TITLE).w,
                            std::ranges::max(std::views::transform(
                                Info, [](const auto &info) {
                                  return (CWinItemExtent(info.Title).w + 8);
                                })));

  Window.Init(w);
  GameFlow.game_main = Proc;
  GameFlow.current_state = GameState::External;
}
} // namespace DAT_MISSING
// -----------------------

void LoaderInit() {
  if (!DAT::Check()) {
    DAT_MISSING::Init();
  } else {
    // Call a different *Init() function here to quickly test a different
    // game state.
    SProjectInit();
  }
}

void LoaderCleanup() {}

// Global variables //
uint32_t MusicNum = 0;                // Number of songs
FaceData face_data[FACE_MAX];         // For face graphics
EndingGrp ending_pic[ENDING_PIC_MAX]; // For endings

// Unused
// static BOOL			bIsBombPalette = FALSE;
// static PALETTEENTRY	tempPalette[256];

static PALETTE EnemyPalette;
PALETTE SProjectPalette;

// Secret function //
static void SetAnimeRect2(ANIME_DATA *anm, int x1, int y1, int x2, int y2);

static int LoadedStage = 0;

// Load graphics for a given stage //
bool LoadGraph(int stage) {
  //	bIsBombPalette = FALSE;
  LoadedStage = stage;
  const auto &graph = DAT::Packfile(DAT::PACK_ID::IMAGES);

  // For music room //
  if (stage == GRAPH_ID_MUSICROOM) {
    return (GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
            GrpBMPLoadP(graph, (19 + 4), SURFACE_ID::MUSIC));
  }
  // For title screen //
  if (stage == GRAPH_ID_TITLE) {
    return (GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
            GrpBMPLoadP(graph, (20 + 4), SURFACE_ID::TITLE));
    // LoadPaletteFrom(SURFACE_ID::ENEMY);
  }
  // For name registration screen //
  if (stage == GRAPH_ID_NAMEREGIST) {
    return (GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
            GrpBMPLoadP(graph, (21 + 4), SURFACE_ID::NAMEREG));
  }
  // For Seihou Project display //
  if (stage == GRAPH_ID_SPROJECT) {
    if (!GrpBMPLoadP(graph, 31, SURFACE_ID::SPROJECT)) {
      return false;
    }
    GrpBackend_PaletteGet(SProjectPalette);

    // if(!GrpBMPLoadP(graph, (21 + 4), SURFACE_ID::NAMEREG)) {
    // 	return false;
    // }
    return true;
  }
  // Load all ending images (including palette) //
  if (stage == GRAPH_ID_ENDING) {
    const auto &in = DAT::Packfile(DAT::PACK_ID::IMAGES);

    if (!GrpBMPLoadP(in, 32, SURFACE_ID::ENDING_CREDITS)) {
      return false;
    }
    for (auto i = 0; i < ENDING_PIC_MAX; i++) {
      if (!GrpBMPLoadP(in, (33 + i), (SURFACE_ID::ENDING_PIC + i))) {
        return false;
      }
      GrpBackend_PaletteGet(ending_pic[i].pal);
    }
    return true;
  }

  // For extra stage system //
  if (stage == GRAPH_ID_EXSTAGE) {
    if (!GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM)) {
      return false;
    }
    if (!GrpBMPLoadP(graph, (27 + 1), SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(EnemyPalette);

    if (!GrpBMPLoadP(graph, 27, SURFACE_ID::MAPCHIP)) {
      return false;
    }

    // For various reasons, it's here //
    if (!GrpBMPLoadP(graph, 26, SURFACE_ID::BOMBER)) {
      return false;
    }

    return true;
  }

  // For extra stage boss (1) //
  if (stage == GRAPH_ID_EXBOSS1) {
    if (!GrpBMPLoadP(graph, 29, SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(EnemyPalette);
    return true;
  }

  // For extra stage boss (2) //
  if (stage == GRAPH_ID_EXBOSS2) {
    if (!GrpBMPLoadP(graph, 30, SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(EnemyPalette);
    return true;
  }

  if ((stage < 0) || (stage > STAGE_MAX)) {
    return false;
  }

  // Map chips will be converted after loading //
  if (!GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM)) {
    return false;
  }
  if (!GrpBMPLoadP(graph, (stage + 0), SURFACE_ID::ENEMY)) {
    return false;
  }
  GrpBackend_PaletteGet(EnemyPalette);

  // Should really be STAGE_MAX
  // const fil_no_t MapChipID[STAGE_MAX] = { 7, 7, 8, 9, 10, 11 };
  const fil_no_t MapChipID[STAGE_MAX] = {7, 8, 9, 10, 11, 12};
  if (!GrpBMPLoadP(graph, MapChipID[stage - 1], SURFACE_ID::MAPCHIP)) {
    return false;
  }

  // For various reasons, it's here //
  return GrpBMPLoadP(graph, 26, SURFACE_ID::BOMBER);
}

void ReloadGraph() {
  assert(LoadedStage != 0);
  LoadGraph(LoadedStage);
}

bool LoadEnemySurface(uint8_t image_no) {
  const auto &graph = DAT::Packfile(DAT::PACK_ID::IMAGES);
  return GrpBMPLoadP(graph, image_no, SURFACE_ID::ENEMY);
}

bool LoadGalleryEnemySurfaces() {
  const auto &graph = DAT::Packfile(DAT::PACK_ID::IMAGES);

  auto bmp29 = BMPLoad(graph.MemExpand(29));
  auto bmp30 = BMPLoad(graph.MemExpand(30));
  if (!bmp29 || !bmp30) {
    return false;
  }

  auto &b29 = bmp29.value();
  auto &b30 = bmp30.value();

  const int src_stride = static_cast<int>(b30.info.Stride());
  const int dst_stride = static_cast<int>(b29.info.Stride());
  const auto src_w = b30.info.biWidth;
  const auto src_h = b30.info.biHeight;
  const auto dst_w = b29.info.biWidth;
  const auto dst_h = b29.info.biHeight;

  const int copy_y = 320;
  const int copy_h = 64;
  const int copy_w = std::min<int>(src_w, dst_w);

  for (int y = 0; y < copy_h; y++) {
    const int src_y = src_h - 1 - (copy_y + y);
    const int dst_y = dst_h - 1 - (copy_y + y);
    if (src_y < 0 || src_y >= src_h || dst_y < 0 || dst_y >= dst_h) {
      continue;
    }
    for (int x = 0; x < copy_w; x++) {
      const auto pixel = b30.pixels[src_y * src_stride + x];
      if (pixel != std::byte{0}) {
        b29.pixels[dst_y * dst_stride + x] = pixel;
      }
    }
  }

  return GrpSurface_Load(SURFACE_ID::ENEMY, std::move(b29));
}

bool LoadFace(uint8_t FaceID, uint8_t FileNo) {
  if (FaceID >= FACE_MAX) {
    return false;
  }
  const auto &graph = DAT::Packfile(DAT::PACK_ID::IMAGES);
  if (!GrpBMPLoadP(graph, (13 + FileNo), (SURFACE_ID::FACE + FaceID))) {
    return false;
  }

  // Save palette //
  GrpBackend_PaletteGet(face_data[FaceID].pal);

  return true;
}

// Set map palette
void LoadPaletteFromMAP() {}

// Load ECL & SCL data into memory //
bool LoadStageData(uint8_t stage) {
  int i = 0;

  // Free memory! //
  Enemies.scl_now = nullptr;
  Enemies.ecl_head = {};
  Enemies.scl_head = {};
  Scroller.scroll.DataHead = nullptr;

  const auto &map_pack = DAT::Packfile(DAT::PACK_ID::MAP);

  // For extra stage system //
  if (stage == GRAPH_ID_EXSTAGE) {
    // ECL Load
    if ((Enemies.ecl_head = LoadEmbeddedScript(24)).data() == nullptr) {
      return false;
    }

    // SCL Load
    if ((Enemies.scl_head = LoadEmbeddedScript(25)).data() == nullptr) {
      return false;
    }

    // MapData Load
    if ((Scroller.scroll.DataHead = map_pack.MemExpand(12)) == nullptr) {
      return false;
    }
  } else if (stage == GRAPH_ID_ENDING) {
    // SCL Load
    if ((Enemies.scl_head = LoadEmbeddedScript(47)).data() == nullptr) {
      return false;
    }
    Enemies.scl_now = Enemies.scl_head.data();
    Games.game_count = 0;
    return true;
  } else {
    // Load each data //
    if ((stage < 1) || (stage > STAGE_MAX)) {
      return false;
    }

    // ECL Load
    if ((Enemies.ecl_head = LoadEmbeddedScript(stage + 0 - 1)).data() ==
        nullptr) {
      return false;
    }

    // SCL Load
    if ((Enemies.scl_head = LoadEmbeddedScript(stage + 6 - 1)).data() ==
        nullptr) {
      return false;
    }

    // MapData Load
    if ((Scroller.scroll.DataHead = map_pack.MemExpand(stage - 1)) == nullptr) {
      return false;
    }
  }

  // Initialize scroll variables //
  if (!Scroller.Init()) {
    return false;
  }

  // Initialize variables //
  Enemies.scl_now = Enemies.scl_head.data();
  Games.game_count = 0;

  // Prepare animations //
  switch (stage) {
  case GRAPH_ID_EXSTAGE: // Extra stage graphics rectangles
    // Extra Boss I //
    // 00 : ■A  0-3   :  Normal (no wings) (10fps)
    Enemies.anime[0].SetSheet<4, 80>({.x = 0, .y = 0}, ANM_NORM);

    // 01 : ■B  4-7   :  Normal (with wings) (10fps)
    Enemies.anime[1].SetSheet<4, 80>({.x = 320, .y = 0}, ANM_NORM);

    // 02 : ■C  8-13  :  Attach wings (no wings -> with wings) (6fps)
    Enemies.anime[2].SetSheet<6, 80>({.x = 0, .y = 80}, ANM_STOP);

    // 03 : ■D  14-15 :  Attack with wings (stationary) (6fps)
    Enemies.anime[3].SetSheet<2, 80>({.x = 480, .y = 80}, ANM_NORM);

    // 04 : ■E  16-17 :  Move with wings (or moving attack) left (6fps)
    Enemies.anime[4].SetSheet<2, 80>({.x = 0, .y = 160}, ANM_NORM);

    // 05 : ■F  18-19 :  Move with wings (or moving attack) right (6fps)
    Enemies.anime[5].SetSheet<2, 80>({.x = 160, .y = 160}, ANM_NORM);

    // 06 : ■G  24-30 :  Gradual change (with wings -> no wings) (6fps)
    Enemies.anime[6].SetSheet<6, 80>({.x = 0, .y = 240}, ANM_STOP);

    // 07 : ■20 : Damage mask for normal state (with/without wings)
    Enemies.anime[7].SetSheet<1, 80>({.x = 320, .y = 160}, ANM_NORM);

    // 08 : ■21 : Damage mask for stationary attack
    Enemies.anime[8].SetSheet<1, 80>({.x = 400, .y = 160}, ANM_NORM);

    // 09 : ■22 : Damage mask for moving (left)
    Enemies.anime[9].SetSheet<1, 80>({.x = 480, .y = 160}, ANM_NORM);

    // 10 : ■23 : Damage mask for moving (right)
    Enemies.anime[10].SetSheet<1, 80>({.x = 560, .y = 160}, ANM_NORM);

    Enemies.anime[11].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 0))},
                                      ANM_NORM);
    Enemies.anime[12].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 1))},
                                      ANM_NORM);
    Enemies.anime[13].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 2))},
                                      ANM_NORM);
    Enemies.anime[14].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 3))},
                                      ANM_NORM);
    Enemies.anime[15].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 4))},
                                      ANM_NORM);
    Enemies.anime[16].SetSheet<4, 32>({.x = (32 * 4), .y = 320}, ANM_NORM);
    Enemies.anime[17].SetSheet<1, 32>({.x = (32 * 4), .y = (320 + (32 * 1))},
                                      ANM_NORM);

    // Extra Boss II //
    // 18 : ■A : Stopped animation (10-12fps)
    Enemies.anime[18].SetSheet<4, 80>({.x = 0, .y = 0}, ANM_NORM);

    // 19 : ■B : Normal phase attack 1 (?fps)
    Enemies.anime[19].SetSheet<4, 80>({.x = 320, .y = 0}, ANM_STOP);

    // 20 : ■C :  Normal phase attack 2 and high-speed move charge pose (6fps)
    Enemies.anime[20].SetSheet<2, 80>({.x = 0, .y = 80}, ANM_NORM);

    // 21 : ■D : Soul state (invincible, not hit by shots) (1-2 fps)
    // (160,80), (200,80), (240,80), (280,80)
    Enemies.anime[21].SetSheet<4, 40>({.x = 160, .y = 80}, ANM_NORM);

    // 22 : ■E : Damage mask (A)
    Enemies.anime[22].SetSheet<1, 80>({.x = 320, .y = 80}, ANM_NORM);

    // 23 : ■E : Damage mask (B)
    Enemies.anime[23].SetSheet<1, 80>({.x = 400, .y = 80}, ANM_NORM);

    // 24 : ■E : Damage mask (G)
    Enemies.anime[24].SetSheet<1, 80>({.x = 480, .y = 80}, ANM_NORM);

    // 25 : ■E : Damage mask (C)
    Enemies.anime[25].SetSheet<1, 80>({.x = 560, .y = 80}, ANM_NORM);

    // 26 : ■F : High-speed movement animation
    Enemies.anime[26].size = {.w = 80, .h = 80};
    Enemies.anime[26].n = 16;
    Enemies.anime[26].mode = ANM_DEG; // Glad it fits in 16 patterns...
    for (i = 0; i < 16; i++) {
      Enemies.anime[26].ptn[i] = PIXEL_LTWH{((i * 80) % 640), 160, 80, 80};
    }

    // 27 : ■G : Normal phase attack 2 charge pose and before/after warp
    Enemies.anime[27].SetSheet<1, 80>({.x = 560, .y = 320}, ANM_NORM);

    // 28-32 : Yin-Yang Orbs x5
    Enemies.anime[28].SetSheet<8, 32>({.x = 0, .y = 384}, ANM_NORM);
    Enemies.anime[29].SetSheet<8, 32>({.x = 0, .y = (384 + 32)}, ANM_NORM);
    Enemies.anime[30].SetSheet<8, 32>({.x = 0, .y = (384 + 64)}, ANM_NORM);
    Enemies.anime[31].SetSheet<8, 32>({.x = 256, .y = (384 + 32)}, ANM_NORM);
    Enemies.anime[32].SetSheet<8, 32>({.x = 256, .y = (384 + 64)}, ANM_NORM);

    Enemies.anime[33].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[34].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[35].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[36].SetSheetDeg<32>({.x = 0, .y = 96});
    Enemies.anime[37].SetSheetDeg<32>({.x = 0, .y = 128});

    // Laser projectile //
    Enemies.anime[38].size = {.w = 40, .h = 56};
    Enemies.anime[38].n = 1;
    Enemies.anime[38].mode = ANM_NORM;
    Enemies.anime[38].ptn[0] = PIXEL_LTWH{512, 0, 40, 56};

    // Mid-boss //
    Enemies.anime[39].size = {.w = 72, .h = 56};
    Enemies.anime[39].n = 2;
    Enemies.anime[39].mode = ANM_NORM;
    Enemies.anime[39].ptn[0] = {0, 424, 72, 480};
    Enemies.anime[39].ptn[1] = {72, 424, (72 * 2), 480};

    // Mid-boss hit //
    Enemies.anime[40].size = {.w = 72, .h = 56};
    Enemies.anime[40].n = 1;
    Enemies.anime[40].mode = ANM_NORM;
    Enemies.anime[40].ptn[0] = {(72 * 2), 424, (72 * 3), 480};

    // Laser projectile hit //
    Enemies.anime[41].size = {.w = 40, .h = 64};
    Enemies.anime[41].n = 1;
    Enemies.anime[41].mode = ANM_NORM;
    Enemies.anime[41].ptn[0] = PIXEL_LTWH{512, 56, 40, 56};

    // Mysterious light bullet //
    Enemies.anime[42].size = {.w = 24, .h = 24};
    Enemies.anime[42].n = 4;
    Enemies.anime[42].mode = ANM_NORM;
    Enemies.anime[42].ptn[0] = PIXEL_LTWH{552, 0, 24, 24};
    Enemies.anime[42].ptn[1] = PIXEL_LTWH{552, 24, 24, 24};
    Enemies.anime[42].ptn[2] = PIXEL_LTWH{552, 0, 24, 24};
    Enemies.anime[42].ptn[3] = PIXEL_LTWH{552, 48, 24, 24};
    break;

  case 1: // Stage 1 graphics rectangles
    // Mid-boss //
    Enemies.anime[0].size = {.w = 72, .h = 56};
    Enemies.anime[0].n = 2;
    Enemies.anime[0].mode = ANM_NORM;
    Enemies.anime[0].ptn[0] = {0, 0, 72, 56};
    Enemies.anime[0].ptn[1] = {72, 0, (72 * 2), 56};

    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = (56 + 0)});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = (56 + 32)});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = (56 + 64)});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = (56 + 96)});

    // Boss //
    Enemies.anime[5].size = {.w = 72, .h = 64};
    Enemies.anime[5].n = 1;
    Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = {0, 184, 72, 248};

    // Mid-boss flash //
    Enemies.anime[6].size = {.w = 72, .h = 56};
    Enemies.anime[6].n = 2;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {(72 * 2), 0, (72 * 3), 56};
    Enemies.anime[6].ptn[1] = {(72 * 3), 0, (72 * 4), 56};

    // Boss flash //
    Enemies.anime[7].size = {.w = 72, .h = 64};
    Enemies.anime[7].n = 1;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {72, 184, (72 * 2), 248};
    break;

  case 2: // Stage 2 graphics rectangles
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 96});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = 128});

    Enemies.anime[5].size = {.w = 112, .h = 48};
    Enemies.anime[5].n = 1;
    Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = {0, 160, 112, 208};

    Enemies.anime[6].size = {.w = 64, .h = 48};
    Enemies.anime[6].n = 1;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {112, 160, 176, 208};

    // Mid-boss //
    Enemies.anime[7].size = {.w = 64, .h = 64};
    Enemies.anime[7].n = 1;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {0, 208, 64, 272};

    // Boss wings //
    Enemies.anime[8].size = {.w = 112, .h = 48};
    Enemies.anime[8].n = 1;
    Enemies.anime[8].mode = ANM_NORM;
    Enemies.anime[8].ptn[0] = {176, 160, 288, 208};

    // Boss orb //
    Enemies.anime[9].size = {.w = 64, .h = 48};
    Enemies.anime[9].n = 1;
    Enemies.anime[9].mode = ANM_NORM;
    Enemies.anime[9].ptn[0] = {288, 160, 352, 208};

    // Boss flash 1 //
    Enemies.anime[10].size = {.w = 112, .h = 48};
    Enemies.anime[10].n = 1;
    Enemies.anime[10].mode = ANM_NORM;
    Enemies.anime[10].ptn[0] = {176, (160 + 48), 288, (208 + 48)};

    // Boss flash 2 //
    Enemies.anime[11].size = {.w = 64, .h = 48};
    Enemies.anime[11].n = 1;
    Enemies.anime[11].mode = ANM_NORM;
    Enemies.anime[11].ptn[0] = {288, (160 + 48), 352, (208 + 48)};

    // Mid-boss flash //
    Enemies.anime[12].size = {.w = 64, .h = 64};
    Enemies.anime[12].n = 1;
    Enemies.anime[12].mode = ANM_NORM;
    Enemies.anime[12].ptn[0] = {(0 + 64), 208, (64 + 64), 272};

    //                            // Winged Left-I //
    //                            Enemies.anime[10].size = { 104, 72 };
    //                            Enemies.anime[10].n      = 1;
    //                            Enemies.anime[10].mode   = ANM_NORM;
    //                            Enemies.anime[10].ptn[0] = { 184, 208, 288,
    //                            280 };
    //
    //                            // Winged Right-I //
    //                            Enemies.anime[11].size = { 104, 72 };
    //                            Enemies.anime[11].n      = 1;
    //                            Enemies.anime[11].mode   = ANM_NORM;
    //                            Enemies.anime[11].ptn[0] = { 288, 208, 392,
    //                            280 };
    //
    //                            // Winged Left-0 //
    //                            Enemies.anime[12].size = { 88, 80 };
    //                            Enemies.anime[12].n      = 1;
    //                            Enemies.anime[12].mode   = ANM_NORM;
    //                            Enemies.anime[12].ptn[0] = { 200, 280, 288,
    //                            360 };
    //
    //                            // Winged Right-0 //
    //                            Enemies.anime[13].size = { 88, 80 };
    //                            Enemies.anime[13].n      = 1;
    //                            Enemies.anime[13].mode   = ANM_NORM;
    //                            Enemies.anime[13].ptn[0] = { 288, 280, 376,
    //                            360 };
    SetAnimeRect2(Enemies.anime + 14, 0, 288, 159, 479);   // Clouds
    SetAnimeRect2(Enemies.anime + 15, 160, 384, 271, 479); //
    SetAnimeRect2(Enemies.anime + 16, 272, 368, 390, 478); //
    SetAnimeRect2(Enemies.anime + 17, 400, 368, 496, 431); //
    SetAnimeRect2(Enemies.anime + 18, 400, 160, 558, 359); //
    SetAnimeRect2(Enemies.anime + 19, 528, 48, 639, 160);  //
    SetAnimeRect2(Enemies.anime + 20, 560, 160, 639, 270); //
    SetAnimeRect2(Enemies.anime + 21, 576, 320, 639, 399); //
    break;

  case 3: // Lord Gates' stage
    Enemies.anime[0].size = {.w = 56, .h = 56};
    Enemies.anime[0].n = 16;
    Enemies.anime[0].mode = ANM_DEG;
    for (i = 0; i < 8; i++) {
      Enemies.anime[0].ptn[i] = PIXEL_LTWH{i * 56, 0, 56, 56};
    }
    for (i = 0; i < 8; i++) {
      Enemies.anime[0].ptn[i + 8] = PIXEL_LTWH{i * 56, 56, 56, 56};
    }

    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 112});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 144});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 176});

    Enemies.anime[4].size = {.w = 48, .h = 16};
    Enemies.anime[4].n = 2;
    Enemies.anime[4].mode = ANM_NORM;
    Enemies.anime[4].ptn[0] = PIXEL_LTWH{592, 0, 48, 16};
    Enemies.anime[4].ptn[1] = PIXEL_LTWH{592, 16, 48, 16};

    Enemies.anime[5].size = {.w = 48, .h = 16};
    Enemies.anime[5].n = 2;
    Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = PIXEL_LTWH{592, 32, 48, 16};
    Enemies.anime[5].ptn[1] = PIXEL_LTWH{592, 48, 48, 16};

    // Boss (464,384)
    Enemies.anime[6].size = {.w = 11 * 16, .h = (5 * 16) + 8};
    Enemies.anime[6].n = 1;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = PIXEL_LTWH{464, 392, (11 * 16), ((5 * 16) + 8)};

    Enemies.anime[7].SetSheetDeg<32>({.x = 0, .y = 208});
    Enemies.anime[8].SetSheetDeg<40>({.x = 0, .y = 240});

    // Boss shadow //
    Enemies.anime[10].size = {.w = 196, .h = 100};
    Enemies.anime[10].n = 1;
    Enemies.anime[10].mode = ANM_NORM;
    Enemies.anime[10].ptn[0] = {444, 292, 640, 392};

    // Boss flash
    Enemies.anime[9].size = {.w = 128, .h = 76};
    Enemies.anime[9].n = 1;
    Enemies.anime[9].mode = ANM_NORM;
    Enemies.anime[9].ptn[0] = {512, 164, 640, 240};
    //	Enemies.anime[9].size = { (11 * 16), ((5 *16) + 8) };
    //            Enemies.anime[9].n      = 1;
    //            Enemies.anime[9].mode   = ANM_NORM;
    //            Enemies.anime[9].ptn[0] = PIXEL_LTWH{ 464, (392 - 88), (11 *
    //            16),
    //       ((5 * 16)
    //       + 8) };
    break;

  case 4: // Marie's stage
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheet<2, 32>({.x = 0, .y = 96}, ANM_NORM);
    Enemies.anime[4].SetSheetDeg<24>({.x = 64, .y = 96});
    Enemies.anime[5].SetSheetDeg<32>({.x = 0, .y = 128});

    //(304,296)-(640,480)
    Enemies.anime[6].size = {.w = (640 - 304), .h = (480 - 296)};
    Enemies.anime[6].n = 1;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {304, 296, 640, 480};

    // Boss flash //
    Enemies.anime[7].size = {.w = (640 - 304 - 32),
                             .h = (480 - 296)}; // Note this
    Enemies.anime[7].n = 1;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {0, 296, 304, 480};
    break;

  case 5:                                               // Master's stage
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0}); // Red one
    Enemies.anime[1].SetSheetDeg<32>(
        {.x = 0, .y = 32}); // Red one appearance effect
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});  // Blue one
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 96});  // Green one
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = 128}); // Orange one
    Enemies.anime[5].SetSheet<4, 32>({.x = 512, .y = 0},
                                     ANM_NORM); // Reactor-equipped bit
    Enemies.anime[6].SetSheet<4, 32>({.x = 512, .y = 64},
                                     ANM_NORM); // Orange one appearance effect

    // Mid-boss options //
    Enemies.anime[7].size = {.w = 24, .h = 24};
    Enemies.anime[7].n = 4;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = PIXEL_LTWH{592, (96 + 0), 24, 24};
    Enemies.anime[7].ptn[1] = PIXEL_LTWH{592, (96 + 24), 24, 24};
    Enemies.anime[7].ptn[2] = PIXEL_LTWH{592, (96 + 0), 24, 24};
    Enemies.anime[7].ptn[3] = PIXEL_LTWH{592, (96 + 48), 24, 24};

    Enemies.anime[8].SetSheet<1>({.x = 512, .y = 96}, {.w = 80, .h = 72},
                                 ANM_NORM); // Sturdy mid-boss

    // Metallic master //
    Enemies.anime[9].SetSheet<1>({.x = 304, .y = 256}, {.w = 336, .h = 224},
                                 ANM_NORM);
    break;

  case 6:
    // Final boss (sitting -> standing) //
    Enemies.anime[0].size = {.w = 56, .h = 72};
    Enemies.anime[0].n = 6;
    Enemies.anime[0].mode = ANM_STOP;
    Enemies.anime[0].ptn[0] = PIXEL_LTWH{(56 * 0), 72, 56, 72};
    Enemies.anime[0].ptn[1] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[0].ptn[2] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[0].ptn[3] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[0].ptn[4] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[0].ptn[5] = PIXEL_LTWH{(56 * 5), 72, 56, 72};

    // Final boss (standing -> sitting) //
    Enemies.anime[1].size = {.w = 56, .h = 72};
    Enemies.anime[1].n = 6;
    Enemies.anime[1].mode = ANM_STOP;
    Enemies.anime[1].ptn[0] = PIXEL_LTWH{(56 * 5), 72, 56, 72};
    Enemies.anime[1].ptn[1] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[1].ptn[2] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[1].ptn[3] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[1].ptn[4] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[1].ptn[5] = PIXEL_LTWH{(56 * 0), 72, 56, 72};

    // Final boss (guard) //
    Enemies.anime[2].size = {.w = 56, .h = 72};
    Enemies.anime[2].n = 4;
    Enemies.anime[2].mode = ANM_NORM;
    Enemies.anime[2].ptn[0] = PIXEL_LTWH{(56 * 6), 72, 56, 72};
    Enemies.anime[2].ptn[1] = PIXEL_LTWH{(56 * 7), 72, 56, 72};
    Enemies.anime[2].ptn[2] = PIXEL_LTWH{(56 * 6), 72, 56, 72};
    Enemies.anime[2].ptn[3] = PIXEL_LTWH{(56 * 8), 72, 56, 72};

    // Final boss (attack 1) //
    Enemies.anime[3].size = {.w = 56, .h = 72};
    Enemies.anime[3].n = 9 + 1;
    Enemies.anime[3].mode = ANM_STOP;
    Enemies.anime[3].ptn[0] = PIXEL_LTWH{(56 * 0), 0, 56, 72};
    Enemies.anime[3].ptn[1] = PIXEL_LTWH{(56 * 1), 0, 56, 72};
    Enemies.anime[3].ptn[2] = PIXEL_LTWH{(56 * 2), 0, 56, 72};
    Enemies.anime[3].ptn[3] = PIXEL_LTWH{(56 * 3), 0, 56, 72};
    Enemies.anime[3].ptn[4] = PIXEL_LTWH{(56 * 4), 0, 56, 72};
    Enemies.anime[3].ptn[5] = PIXEL_LTWH{(56 * 5), 0, 56, 72};
    Enemies.anime[3].ptn[6] = PIXEL_LTWH{(56 * 6), 0, 56, 72};
    Enemies.anime[3].ptn[7] = PIXEL_LTWH{(56 * 7), 0, 56, 72};
    Enemies.anime[3].ptn[8] = PIXEL_LTWH{(56 * 8), 0, 56, 72};
    Enemies.anime[3].ptn[9] = PIXEL_LTWH{(56 * 5), 72, 56, 72}; // Added extra

    // Larva stage //
    SetAnimeRect2(Enemies.anime + 4, 432, 272, 632, 464);

    // Final boss (hope it looks like a jump) //
    Enemies.anime[5].size = {.w = 56, .h = 72};
    Enemies.anime[5].n = 11;
    Enemies.anime[5].mode = ANM_STOP;
    Enemies.anime[5].ptn[0] = PIXEL_LTWH{(56 * 0), 72, 56, 72};
    Enemies.anime[5].ptn[1] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[5].ptn[2] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[5].ptn[3] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[5].ptn[4] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[5].ptn[5] = PIXEL_LTWH{(56 * 5), 72, 56, 72};
    Enemies.anime[5].ptn[6] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[5].ptn[7] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[5].ptn[8] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[5].ptn[9] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[5].ptn[10] = PIXEL_LTWH{(56 * 0), 72, 56, 72};

    // Bits released in butterfly state? (Open) //
    Enemies.anime[6].size = {.w = 33, .h = 32};
    Enemies.anime[6].n = 10;
    Enemies.anime[6].mode = ANM_STOP;
    Enemies.anime[6].ptn[0] = PIXEL_LTWH{(32 * 0), 416, 32, 32};
    Enemies.anime[6].ptn[1] = PIXEL_LTWH{(32 * 1), 416, 32, 32};
    Enemies.anime[6].ptn[2] = PIXEL_LTWH{(32 * 2), 416, 32, 32};
    Enemies.anime[6].ptn[3] = PIXEL_LTWH{(32 * 3), 416, 32, 32};
    Enemies.anime[6].ptn[4] = PIXEL_LTWH{(32 * 4), 416, 32, 32};
    Enemies.anime[6].ptn[5] = PIXEL_LTWH{(32 * 5), 416, 32, 32};
    Enemies.anime[6].ptn[6] = PIXEL_LTWH{(32 * 0), 448, 32, 32};
    Enemies.anime[6].ptn[7] = PIXEL_LTWH{(32 * 1), 448, 32, 32};
    Enemies.anime[6].ptn[8] = PIXEL_LTWH{(32 * 2), 448, 32, 32};
    Enemies.anime[6].ptn[9] = PIXEL_LTWH{(32 * 3), 448, 32, 32};

    // Bits released in butterfly state? (Close) //
    Enemies.anime[7].size = {.w = 33, .h = 32};
    Enemies.anime[7].n = 10;
    Enemies.anime[7].mode = ANM_STOP;
    Enemies.anime[7].ptn[0] = PIXEL_LTWH{(32 * 3), 448, 32, 32};
    Enemies.anime[7].ptn[1] = PIXEL_LTWH{(32 * 2), 448, 32, 32};
    Enemies.anime[7].ptn[2] = PIXEL_LTWH{(32 * 1), 448, 32, 32};
    Enemies.anime[7].ptn[3] = PIXEL_LTWH{(32 * 0), 448, 32, 32};
    Enemies.anime[7].ptn[4] = PIXEL_LTWH{(32 * 5), 416, 32, 32};
    Enemies.anime[7].ptn[5] = PIXEL_LTWH{(32 * 4), 416, 32, 32};
    Enemies.anime[7].ptn[6] = PIXEL_LTWH{(32 * 3), 416, 32, 32};
    Enemies.anime[7].ptn[7] = PIXEL_LTWH{(32 * 2), 416, 32, 32};
    Enemies.anime[7].ptn[8] = PIXEL_LTWH{(32 * 1), 416, 32, 32};
    Enemies.anime[7].ptn[9] = PIXEL_LTWH{(32 * 0), 416, 32, 32};

    Enemies.anime[8].SetSheet<1>({.x = 0, .y = 368}, {.w = 48, .h = 48},
                                 ANM_NORM); // Sturdy mid-boss
    break;
  }

  return true;
}

// Define a single pattern graphic as an animation //
static void SetAnimeRect2(ANIME_DATA *anm, int x1, int y1, int x2, int y2) {
  anm->size = {.w = (x2 - x1), .h = (y2 - y1)};
  anm->n = 1;
  anm->mode = ANM_NORM;

  anm->ptn[0] = {x1, y1, x2, y2};
}

// Load the n-th song //
bool LoadMusic(unsigned int no) { return DAT::LoadMusic(no); }

bool LoadMusicByIndex(int index) { return DAT::LoadMusicByIndex(index); }

// Load all Sound data //
bool LoadSound(const PACKFILE_READ &in) {
  // Initialize sound //
  // Disable if unavailable for any reason //
  if ((!ConfigDat.se_enabled) || !Snd_SEInit()) {
    ConfigDat.se_enabled = false;
    return false;
  }

  while (1) {
    if (!Snd_SELoadP(in, SOUND_ID_KEBARI, SOUND_ID_KEBARI, SNDMAX_KEBARI)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_TAME, SOUND_ID_TAME, SNDMAX_TAME)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_LASER, SOUND_ID_LASER, SNDMAX_LASER)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_LASER2, SOUND_ID_LASER2, SNDMAX_LASER2)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_BOMB, SOUND_ID_BOMB, SNDMAX_BOMB)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_SELECT, SOUND_ID_SELECT, SNDMAX_SELECT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_HIT, SOUND_ID_HIT, SNDMAX_HIT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_CANCEL, SOUND_ID_CANCEL, SNDMAX_CANCEL)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_WARNING, SOUND_ID_WARNING, SNDMAX_WARNING)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_SBLASER, SOUND_ID_SBLASER, SNDMAX_SBLASER)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_BUZZ, SOUND_ID_BUZZ, SNDMAX_BUZZ)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_MISSILE, SOUND_ID_MISSILE, SNDMAX_MISSILE)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_JOINT, SOUND_ID_JOINT, SNDMAX_JOINT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_DEAD, SOUND_ID_DEAD, SNDMAX_DEAD)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_SBBOMB, SOUND_ID_SBBOMB, SNDMAX_SBBOMB)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_BOSSBOMB, SOUND_ID_BOSSBOMB,
                     SNDMAX_BOSSBOMB)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_ENEMYSHOT, SOUND_ID_ENEMYSHOT,
                     SNDMAX_ENEMYSHOT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_HLASER, SOUND_ID_HLASER, SNDMAX_HLASER)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_TAMEFAST, SOUND_ID_TAMEFAST,
                     SNDMAX_TAMEFAST)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_WARP, SOUND_ID_WARP, SNDMAX_WARP)) {
      break;
    }

    // DirectSound can only apply volume onto loaded buffers.
    Snd_UpdateVolumes();
    return true;
  }

  ConfigDat.se_enabled = false;
  Snd_SECleanup();
  return false;
}

bool LoadSound() { return LoadSound(DAT::Packfile(DAT::PACK_ID::SOUND)); }

BYTE_BUFFER_OWNED LoadMusicRoomComment(int no) {
  if ((no < 0) || (no >= static_cast<int>(DAT::MusicMetas.size()))) {
    return nullptr;
  }
  const auto &comment = DAT::MusicMetas[no].comment;
  BYTE_BUFFER_OWNED buf(comment.size());
  if (buf) {
    std::memcpy(buf.get(), comment.data(), comment.size());
  }
  return buf;
}

std::string_view MusicTitle(unsigned int index) {
  if (index < DAT::MusicMetas.size()) {
    return DAT::MusicMetas[index].title;
  }
  return {};
}

std::string_view MusicComment(unsigned int index) {
  if (index < DAT::MusicMetas.size()) {
    return DAT::MusicMetas[index].comment;
  }
  return {};
}

BYTE_BUFFER_OWNED LoadDemo(int stage) {
  return DAT::Packfile(DAT::PACK_ID::MAP).MemExpand(stage - 1 + 6);
}
