///
/// Loader - Load graphics, sounds, etc.
///

#pragma once

#include "constants.h"
#include "game/coords.h"
#include "game/graphics.h"
#include "game/hash.h"

struct SURFACE_DDRAW;

// Constants
inline constexpr auto FACE_NUMX = 6; // Number of face graphic columns

// Special graphics IDs (for LoadGraph())
inline constexpr auto GRAPH_ID_MUSICROOM =
    128; // Music room BMP ID (should not be 1-6)
inline constexpr auto GRAPH_ID_TITLE = (128 + 1); // Title screen BMP ID
inline constexpr auto GRAPH_ID_NAMEREGIST =
    (128 + 2); // Name registration screen BMP ID
inline constexpr auto GRAPH_ID_EXSTAGE =
    (128 + 3); // Extra stage system
inline constexpr auto GRAPH_ID_EXBOSS1 =
    (128 + 4); // Extra stage boss 1
inline constexpr auto GRAPH_ID_EXBOSS2 =
    (128 + 5); // Extra stage boss 2
inline constexpr auto GRAPH_ID_SPROJECT = (128 + 6); // Seihou Project display
inline constexpr auto GRAPH_ID_ENDING = (128 + 7); // Load ending graphics

// Sound effects
inline constexpr auto SOUND_ID_KEBARI = 0x00;
inline constexpr auto SOUND_ID_TAME = 0x01;
inline constexpr auto SOUND_ID_LASER = 0x02;
inline constexpr auto SOUND_ID_LASER2 = 0x03;
inline constexpr auto SOUND_ID_BOMB = 0x04;
inline constexpr auto SOUND_ID_SELECT = 0x05;
inline constexpr auto SOUND_ID_HIT = 0x06;
inline constexpr auto SOUND_ID_CANCEL = 0x07;
inline constexpr auto SOUND_ID_WARNING = 0x08;
inline constexpr auto SOUND_ID_SBLASER = 0x09;
inline constexpr auto SOUND_ID_BUZZ = 0x0a;
inline constexpr auto SOUND_ID_MISSILE = 0x0b;
inline constexpr auto SOUND_ID_JOINT = 0x0c;
inline constexpr auto SOUND_ID_DEAD = 0x0d;
inline constexpr auto SOUND_ID_SBBOMB = 0x0e;
inline constexpr auto SOUND_ID_BOSSBOMB = 0x0f;
inline constexpr auto SOUND_ID_ENEMYSHOT = 0x10;
inline constexpr auto SOUND_ID_HLASER = 0x11;
inline constexpr auto SOUND_ID_TAMEFAST = 0x12;
inline constexpr auto SOUND_ID_WARP = 0x13;

// Max simultaneous sounds
inline constexpr auto SNDMAX_KEBARI = 5;
inline constexpr auto SNDMAX_TAME = 5;
inline constexpr auto SNDMAX_LASER = 1;
inline constexpr auto SNDMAX_LASER2 = 1;
inline constexpr auto SNDMAX_BOMB = 1; // 5
inline constexpr auto SNDMAX_SELECT = 1;
inline constexpr auto SNDMAX_HIT = 1; // 5
inline constexpr auto SNDMAX_CANCEL = 1;
inline constexpr auto SNDMAX_WARNING = 1;
inline constexpr auto SNDMAX_SBLASER = 1;
inline constexpr auto SNDMAX_BUZZ = 2; // 2
inline constexpr auto SNDMAX_MISSILE = 5;
inline constexpr auto SNDMAX_JOINT = 1;
inline constexpr auto SNDMAX_DEAD = 1;
inline constexpr auto SNDMAX_SBBOMB = 1;
inline constexpr auto SNDMAX_BOSSBOMB = 1;
inline constexpr auto SNDMAX_ENEMYSHOT = 5;
inline constexpr auto SNDMAX_HLASER = 1;
inline constexpr auto SNDMAX_TAMEFAST = 5;
inline constexpr auto SNDMAX_WARP = 1;

struct FaceData {
  PALETTE pal; // Palette for face graphics
};
// (FACE_DATA alias removed — use FaceData directly)

// Ending graphics management
struct EndingGrp {
  PIXEL_LTRB rcTarget; // Rectangle bounds
  PALETTE pal;         // Palette
};
// (ENDING_GRP alias removed — use EndingGrp directly)

// Functions
void LoaderInit();
void LoaderCleanup();
[[nodiscard]] bool
LoadStageData(uint8_t stage); // Load ECL & SCL data to memory
[[nodiscard]] bool
LoadGraph(int stage); // Load stage graphics
[[nodiscard]] bool LoadFace(uint8_t FaceID,
                            uint8_t FileNo);   // Load face graphic
[[nodiscard]] bool LoadMusic(unsigned int no); // Load nth music track
[[nodiscard]] bool LoadMusicByHash(const HASH &hash);
[[nodiscard]] bool LoadMIDIBuffer(BYTE_BUFFER_OWNED /*buf*/);
bool LoadSound(); // Load all sound data

// Load Music Room comment
BYTE_BUFFER_OWNED LoadMusicRoomComment(int no);

// Access unified music metadata (title/comment cached from MUSIC.PAK)
[[nodiscard]] std::string_view MusicTitle(unsigned int index);
[[nodiscard]] std::string_view MusicComment(unsigned int index);

BYTE_BUFFER_OWNED LoadDemo(int stage);

void LoadPaletteFromMAP(); // Load map palette

// Reloads the last stage loaded with LoadGraph().
void ReloadGraph();

// Variables

// Face graphics
extern const std::reference_wrapper<SURFACE_DDRAW> GrFaces[FACE_MAX];

extern const std::reference_wrapper<SURFACE_DDRAW> GrEndingPic[ENDING_PIC_MAX];

extern FaceData face_data[FACE_MAX]; // Face graphics data

extern uint32_t MusicNum; // Number of music tracks

extern PALETTE SProjectPalette;

extern EndingGrp ending_pic[ENDING_PIC_MAX];
