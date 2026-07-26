///
/// anime_data — per-stage enemy animation sprite sheet configuration
///
#include "anime_data.h"

#include "enemy/enemy.h"
#include "enemy/enemy_manager.h"

namespace {

void SetAnimeRect(ANIME_DATA *anm, int x1, int y1, int x2, int y2) {
  anm->size = {.w = (x2 - x1), .h = (y2 - y1)};
  anm->n = 1;
  anm->mode = ANM_NORM;
  anm->ptn[0] = {x1, y1, x2, y2};
}

} // namespace

namespace anime_data {

void SetupStageAnime(StageId stage_num) {
  int i = 0;

  switch (stage_num) {
  case StageId::EXTRA:
    Enemies.anime[0].SetSheet<4, 80>({.x = 0, .y = 0}, ANM_NORM);
    Enemies.anime[1].SetSheet<4, 80>({.x = 320, .y = 0}, ANM_NORM);
    Enemies.anime[2].SetSheet<6, 80>({.x = 0, .y = 80}, ANM_STOP);
    Enemies.anime[3].SetSheet<2, 80>({.x = 480, .y = 80}, ANM_NORM);
    Enemies.anime[4].SetSheet<2, 80>({.x = 0, .y = 160}, ANM_NORM);
    Enemies.anime[5].SetSheet<2, 80>({.x = 160, .y = 160}, ANM_NORM);
    Enemies.anime[6].SetSheet<6, 80>({.x = 0, .y = 240}, ANM_STOP);
    Enemies.anime[7].SetSheet<1, 80>({.x = 320, .y = 160}, ANM_NORM);
    Enemies.anime[8].SetSheet<1, 80>({.x = 400, .y = 160}, ANM_NORM);
    Enemies.anime[9].SetSheet<1, 80>({.x = 480, .y = 160}, ANM_NORM);
    Enemies.anime[10].SetSheet<1, 80>({.x = 560, .y = 160}, ANM_NORM);
    Enemies.anime[11].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 0))}, ANM_NORM);
    Enemies.anime[12].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 1))}, ANM_NORM);
    Enemies.anime[13].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 2))}, ANM_NORM);
    Enemies.anime[14].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 3))}, ANM_NORM);
    Enemies.anime[15].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 4))}, ANM_NORM);
    Enemies.anime[16].SetSheet<4, 32>({.x = (32 * 4), .y = 320}, ANM_NORM);
    Enemies.anime[17].SetSheet<1, 32>({.x = (32 * 4), .y = (320 + (32 * 1))}, ANM_NORM);
    Enemies.anime[18].SetSheet<4, 80>({.x = 0, .y = 0}, ANM_NORM);
    Enemies.anime[19].SetSheet<4, 80>({.x = 320, .y = 0}, ANM_STOP);
    Enemies.anime[20].SetSheet<2, 80>({.x = 0, .y = 80}, ANM_NORM);
    Enemies.anime[21].SetSheet<4, 40>({.x = 160, .y = 80}, ANM_NORM);
    Enemies.anime[22].SetSheet<1, 80>({.x = 320, .y = 80}, ANM_NORM);
    Enemies.anime[23].SetSheet<1, 80>({.x = 400, .y = 80}, ANM_NORM);
    Enemies.anime[24].SetSheet<1, 80>({.x = 480, .y = 80}, ANM_NORM);
    Enemies.anime[25].SetSheet<1, 80>({.x = 560, .y = 80}, ANM_NORM);
    Enemies.anime[26].size = {.w = 80, .h = 80};
    Enemies.anime[26].n = 16;
    Enemies.anime[26].mode = ANM_DEG;
    for (i = 0; i < 16; i++)
      Enemies.anime[26].ptn[i] = PIXEL_LTWH{((i * 80) % 640), 160, 80, 80};
    Enemies.anime[27].SetSheet<1, 80>({.x = 560, .y = 320}, ANM_NORM);
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
    Enemies.anime[38].size = {.w = 40, .h = 56};
    Enemies.anime[38].n = 1; Enemies.anime[38].mode = ANM_NORM;
    Enemies.anime[38].ptn[0] = PIXEL_LTWH{512, 0, 40, 56};
    Enemies.anime[39].size = {.w = 72, .h = 56};
    Enemies.anime[39].n = 2; Enemies.anime[39].mode = ANM_NORM;
    Enemies.anime[39].ptn[0] = {0, 424, 72, 480};
    Enemies.anime[39].ptn[1] = {72, 424, (72 * 2), 480};
    Enemies.anime[40].size = {.w = 72, .h = 56};
    Enemies.anime[40].n = 1; Enemies.anime[40].mode = ANM_NORM;
    Enemies.anime[40].ptn[0] = {(72 * 2), 424, (72 * 3), 480};
    Enemies.anime[41].size = {.w = 40, .h = 64};
    Enemies.anime[41].n = 1; Enemies.anime[41].mode = ANM_NORM;
    Enemies.anime[41].ptn[0] = PIXEL_LTWH{512, 56, 40, 56};
    Enemies.anime[42].size = {.w = 24, .h = 24};
    Enemies.anime[42].n = 4; Enemies.anime[42].mode = ANM_NORM;
    Enemies.anime[42].ptn[0] = PIXEL_LTWH{552, 0, 24, 24};
    Enemies.anime[42].ptn[1] = PIXEL_LTWH{552, 24, 24, 24};
    Enemies.anime[42].ptn[2] = PIXEL_LTWH{552, 0, 24, 24};
    Enemies.anime[42].ptn[3] = PIXEL_LTWH{552, 48, 24, 24};
    break;
  case StageId::STAGE_1:
    Enemies.anime[0].size = {.w = 72, .h = 56};
    Enemies.anime[0].n = 2; Enemies.anime[0].mode = ANM_NORM;
    Enemies.anime[0].ptn[0] = {0, 0, 72, 56};
    Enemies.anime[0].ptn[1] = {72, 0, (72 * 2), 56};
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = (56 + 0)});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = (56 + 32)});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = (56 + 64)});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = (56 + 96)});
    Enemies.anime[5].size = {.w = 72, .h = 64};
    Enemies.anime[5].n = 1; Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = {0, 184, 72, 248};
    Enemies.anime[6].size = {.w = 72, .h = 56};
    Enemies.anime[6].n = 2; Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {(72 * 2), 0, (72 * 3), 56};
    Enemies.anime[6].ptn[1] = {(72 * 3), 0, (72 * 4), 56};
    Enemies.anime[7].size = {.w = 72, .h = 64};
    Enemies.anime[7].n = 1; Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {72, 184, (72 * 2), 248};
    break;
  case StageId::STAGE_2:
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 96});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = 128});
    Enemies.anime[5].size = {.w = 112, .h = 48};
    Enemies.anime[5].n = 1; Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = {0, 160, 112, 208};
    Enemies.anime[6].size = {.w = 64, .h = 48};
    Enemies.anime[6].n = 1; Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {112, 160, 176, 208};
    Enemies.anime[7].size = {.w = 64, .h = 64};
    Enemies.anime[7].n = 1; Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {0, 208, 64, 272};
    Enemies.anime[8].size = {.w = 112, .h = 48};
    Enemies.anime[8].n = 1; Enemies.anime[8].mode = ANM_NORM;
    Enemies.anime[8].ptn[0] = {176, 160, 288, 208};
    Enemies.anime[9].size = {.w = 64, .h = 48};
    Enemies.anime[9].n = 1; Enemies.anime[9].mode = ANM_NORM;
    Enemies.anime[9].ptn[0] = {288, 160, 352, 208};
    Enemies.anime[10].size = {.w = 112, .h = 48};
    Enemies.anime[10].n = 1; Enemies.anime[10].mode = ANM_NORM;
    Enemies.anime[10].ptn[0] = {176, (160 + 48), 288, (208 + 48)};
    Enemies.anime[11].size = {.w = 64, .h = 48};
    Enemies.anime[11].n = 1; Enemies.anime[11].mode = ANM_NORM;
    Enemies.anime[11].ptn[0] = {288, (160 + 48), 352, (208 + 48)};
    Enemies.anime[12].size = {.w = 64, .h = 64};
    Enemies.anime[12].n = 1; Enemies.anime[12].mode = ANM_NORM;
    Enemies.anime[12].ptn[0] = {64, 208, 128, 272};
    SetAnimeRect(&Enemies.anime[14], 0, 288, 159, 479);
    SetAnimeRect(&Enemies.anime[15], 160, 384, 271, 479);
    SetAnimeRect(&Enemies.anime[16], 272, 368, 390, 478);
    SetAnimeRect(&Enemies.anime[17], 400, 368, 496, 431);
    SetAnimeRect(&Enemies.anime[18], 400, 160, 558, 359);
    SetAnimeRect(&Enemies.anime[19], 528, 48, 639, 160);
    SetAnimeRect(&Enemies.anime[20], 560, 160, 639, 270);
    SetAnimeRect(&Enemies.anime[21], 576, 320, 639, 399);
    break;
  case StageId::STAGE_3:
    Enemies.anime[0].size = {.w = 56, .h = 56};
    Enemies.anime[0].n = 16; Enemies.anime[0].mode = ANM_DEG;
    for (i = 0; i < 8; i++) Enemies.anime[0].ptn[i] = PIXEL_LTWH{i * 56, 0, 56, 56};
    for (i = 0; i < 8; i++) Enemies.anime[0].ptn[i + 8] = PIXEL_LTWH{i * 56, 56, 56, 56};
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 112});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 144});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 176});
    Enemies.anime[4].size = {.w = 48, .h = 16};
    Enemies.anime[4].n = 2; Enemies.anime[4].mode = ANM_NORM;
    Enemies.anime[4].ptn[0] = PIXEL_LTWH{592, 0, 48, 16};
    Enemies.anime[4].ptn[1] = PIXEL_LTWH{592, 16, 48, 16};
    Enemies.anime[5].size = {.w = 48, .h = 16};
    Enemies.anime[5].n = 2; Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = PIXEL_LTWH{592, 32, 48, 16};
    Enemies.anime[5].ptn[1] = PIXEL_LTWH{592, 48, 48, 16};
    Enemies.anime[6].size = {.w = 11 * 16, .h = (5 * 16) + 8};
    Enemies.anime[6].n = 1; Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = PIXEL_LTWH{464, 392, (11 * 16), ((5 * 16) + 8)};
    Enemies.anime[7].SetSheetDeg<32>({.x = 0, .y = 208});
    Enemies.anime[8].SetSheetDeg<40>({.x = 0, .y = 240});
    Enemies.anime[10].size = {.w = 196, .h = 100};
    Enemies.anime[10].n = 1; Enemies.anime[10].mode = ANM_NORM;
    Enemies.anime[10].ptn[0] = {444, 292, 640, 392};
    Enemies.anime[9].size = {.w = 128, .h = 76};
    Enemies.anime[9].n = 1; Enemies.anime[9].mode = ANM_NORM;
    Enemies.anime[9].ptn[0] = {512, 164, 640, 240};
    break;
  case StageId::STAGE_4:
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheet<2, 32>({.x = 0, .y = 96}, ANM_NORM);
    Enemies.anime[4].SetSheetDeg<24>({.x = 64, .y = 96});
    Enemies.anime[5].SetSheetDeg<32>({.x = 0, .y = 128});
    Enemies.anime[6].size = {.w = (640 - 304), .h = (480 - 296)};
    Enemies.anime[6].n = 1; Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {304, 296, 640, 480};
    Enemies.anime[7].size = {.w = (640 - 304 - 32), .h = (480 - 296)};
    Enemies.anime[7].n = 1; Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {0, 296, 304, 480};
    break;
  case StageId::STAGE_5:
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 96});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = 128});
    Enemies.anime[5].SetSheet<4, 32>({.x = 512, .y = 0}, ANM_NORM);
    Enemies.anime[6].SetSheet<4, 32>({.x = 512, .y = 64}, ANM_NORM);
    Enemies.anime[7].size = {.w = 24, .h = 24};
    Enemies.anime[7].n = 4; Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = PIXEL_LTWH{592, (96 + 0), 24, 24};
    Enemies.anime[7].ptn[1] = PIXEL_LTWH{592, (96 + 24), 24, 24};
    Enemies.anime[7].ptn[2] = PIXEL_LTWH{592, (96 + 0), 24, 24};
    Enemies.anime[7].ptn[3] = PIXEL_LTWH{592, (96 + 48), 24, 24};
    Enemies.anime[8].SetSheet<1>({.x = 512, .y = 96}, {.w = 80, .h = 72}, ANM_NORM);
    Enemies.anime[9].SetSheet<1>({.x = 304, .y = 256}, {.w = 336, .h = 224}, ANM_NORM);
    break;
  case StageId::STAGE_6:
    Enemies.anime[0].size = {.w = 56, .h = 72};
    Enemies.anime[0].n = 6; Enemies.anime[0].mode = ANM_STOP;
    for (i = 0; i < 6; i++) Enemies.anime[0].ptn[i] = PIXEL_LTWH{(56 * i), 72, 56, 72};
    Enemies.anime[1].size = {.w = 56, .h = 72};
    Enemies.anime[1].n = 6; Enemies.anime[1].mode = ANM_STOP;
    for (i = 0; i < 6; i++) Enemies.anime[1].ptn[i] = PIXEL_LTWH{(56 * (5 - i)), 72, 56, 72};
    Enemies.anime[2].size = {.w = 56, .h = 72};
    Enemies.anime[2].n = 4; Enemies.anime[2].mode = ANM_NORM;
    Enemies.anime[2].ptn[0] = PIXEL_LTWH{(56 * 6), 72, 56, 72};
    Enemies.anime[2].ptn[1] = PIXEL_LTWH{(56 * 7), 72, 56, 72};
    Enemies.anime[2].ptn[2] = PIXEL_LTWH{(56 * 6), 72, 56, 72};
    Enemies.anime[2].ptn[3] = PIXEL_LTWH{(56 * 8), 72, 56, 72};
    Enemies.anime[3].size = {.w = 56, .h = 72};
    Enemies.anime[3].n = 10; Enemies.anime[3].mode = ANM_STOP;
    for (i = 0; i < 9; i++) Enemies.anime[3].ptn[i] = PIXEL_LTWH{(56 * i), 0, 56, 72};
    Enemies.anime[3].ptn[9] = PIXEL_LTWH{(56 * 5), 72, 56, 72};
    SetAnimeRect(&Enemies.anime[4], 432, 272, 632, 464);
    Enemies.anime[5].size = {.w = 56, .h = 72};
    Enemies.anime[5].n = 11; Enemies.anime[5].mode = ANM_STOP;
    for (i = 0; i < 6; i++) Enemies.anime[5].ptn[i] = PIXEL_LTWH{(56 * i), 72, 56, 72};
    for (i = 0; i < 5; i++) Enemies.anime[5].ptn[i + 6] = PIXEL_LTWH{(56 * (4 - i)), 72, 56, 72};
    Enemies.anime[6].size = {.w = 33, .h = 32};
    Enemies.anime[6].n = 10; Enemies.anime[6].mode = ANM_STOP;
    for (i = 0; i < 6; i++) Enemies.anime[6].ptn[i] = PIXEL_LTWH{(32 * i), 416, 32, 32};
    for (i = 0; i < 4; i++) Enemies.anime[6].ptn[i + 6] = PIXEL_LTWH{(32 * i), 448, 32, 32};
    Enemies.anime[7].size = {.w = 33, .h = 32};
    Enemies.anime[7].n = 10; Enemies.anime[7].mode = ANM_STOP;
    for (i = 0; i < 4; i++) Enemies.anime[7].ptn[i] = PIXEL_LTWH{(32 * (3 - i)), 448, 32, 32};
    for (i = 0; i < 6; i++) Enemies.anime[7].ptn[i + 4] = PIXEL_LTWH{(32 * (5 - i)), 416, 32, 32};
    Enemies.anime[8].SetSheet<1>({.x = 0, .y = 368}, {.w = 48, .h = 48}, ANM_NORM);
    break;
  }
}

} // namespace anime_data
