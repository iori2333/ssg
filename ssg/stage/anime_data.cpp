///
/// anime_data — per-stage enemy animation sprite sheet configuration
///

#include "anime_data.h"

#include "enemy/actor/enemy_actor.h"
#include "gameplay/game_rules.h"

namespace {

void SetAnimeRect(EnemyAnimation &animation, int x1, int y1, int x2, int y2) {
  animation.size = {.x = (x2 - x1), .y = (y2 - y1)};
  animation.n = 1;
  animation.mode = EnemyAnimationMode::Loop;
  animation.ptn[0] = {x1, y1, x2, y2};
}

} // namespace

namespace anime_data {

void SetupStageAnime(StageId stage_num, EnemyAnimationSet &animations) {
  int i = 0;

  switch (stage_num) {
  case StageId::Extra:
    animations[0].SetSquareSheet({.x = 0, .y = 0}, 4, 80,
                                 EnemyAnimationMode::Loop);
    animations[1].SetSquareSheet({.x = 320, .y = 0}, 4, 80,
                                 EnemyAnimationMode::Loop);
    animations[2].SetSquareSheet({.x = 0, .y = 80}, 6, 80,
                                 EnemyAnimationMode::StopAtEnd);
    animations[3].SetSquareSheet({.x = 480, .y = 80}, 2, 80,
                                 EnemyAnimationMode::Loop);
    animations[4].SetSquareSheet({.x = 0, .y = 160}, 2, 80,
                                 EnemyAnimationMode::Loop);
    animations[5].SetSquareSheet({.x = 160, .y = 160}, 2, 80,
                                 EnemyAnimationMode::Loop);
    animations[6].SetSquareSheet({.x = 0, .y = 240}, 6, 80,
                                 EnemyAnimationMode::StopAtEnd);
    animations[7].SetSquareSheet({.x = 320, .y = 160}, 1, 80,
                                 EnemyAnimationMode::Loop);
    animations[8].SetSquareSheet({.x = 400, .y = 160}, 1, 80,
                                 EnemyAnimationMode::Loop);
    animations[9].SetSquareSheet({.x = 480, .y = 160}, 1, 80,
                                 EnemyAnimationMode::Loop);
    animations[10].SetSquareSheet({.x = 560, .y = 160}, 1, 80,
                                  EnemyAnimationMode::Loop);
    animations[11].SetSquareSheet({.x = 0, .y = (320 + (32 * 0))}, 4, 32,
                                  EnemyAnimationMode::Loop);
    animations[12].SetSquareSheet({.x = 0, .y = (320 + (32 * 1))}, 4, 32,
                                  EnemyAnimationMode::Loop);
    animations[13].SetSquareSheet({.x = 0, .y = (320 + (32 * 2))}, 4, 32,
                                  EnemyAnimationMode::Loop);
    animations[14].SetSquareSheet({.x = 0, .y = (320 + (32 * 3))}, 4, 32,
                                  EnemyAnimationMode::Loop);
    animations[15].SetSquareSheet({.x = 0, .y = (320 + (32 * 4))}, 4, 32,
                                  EnemyAnimationMode::Loop);
    animations[16].SetSquareSheet({.x = (32 * 4), .y = 320}, 4, 32,
                                  EnemyAnimationMode::Loop);
    animations[17].SetSquareSheet({.x = (32 * 4), .y = (320 + (32 * 1))}, 1, 32,
                                  EnemyAnimationMode::Loop);
    animations[18].SetSquareSheet({.x = 0, .y = 0}, 4, 80,
                                  EnemyAnimationMode::Loop);
    animations[19].SetSquareSheet({.x = 320, .y = 0}, 4, 80,
                                  EnemyAnimationMode::StopAtEnd);
    animations[20].SetSquareSheet({.x = 0, .y = 80}, 2, 80,
                                  EnemyAnimationMode::Loop);
    animations[21].SetSquareSheet({.x = 160, .y = 80}, 4, 40,
                                  EnemyAnimationMode::Loop);
    animations[22].SetSquareSheet({.x = 320, .y = 80}, 1, 80,
                                  EnemyAnimationMode::Loop);
    animations[23].SetSquareSheet({.x = 400, .y = 80}, 1, 80,
                                  EnemyAnimationMode::Loop);
    animations[24].SetSquareSheet({.x = 480, .y = 80}, 1, 80,
                                  EnemyAnimationMode::Loop);
    animations[25].SetSquareSheet({.x = 560, .y = 80}, 1, 80,
                                  EnemyAnimationMode::Loop);
    animations[26].size = {.x = 80, .y = 80};
    animations[26].n = 16;
    animations[26].mode = EnemyAnimationMode::Directional;
    for (i = 0; i < 16; i++) {
      animations[26].ptn[i] = Rect::FromLtwh(((i * 80) % 640), 160, 80, 80);
    }
    animations[27].SetSquareSheet({.x = 560, .y = 320}, 1, 80,
                                  EnemyAnimationMode::Loop);
    animations[28].SetSquareSheet({.x = 0, .y = 384}, 8, 32,
                                  EnemyAnimationMode::Loop);
    animations[29].SetSquareSheet({.x = 0, .y = (384 + 32)}, 8, 32,
                                  EnemyAnimationMode::Loop);
    animations[30].SetSquareSheet({.x = 0, .y = (384 + 64)}, 8, 32,
                                  EnemyAnimationMode::Loop);
    animations[31].SetSquareSheet({.x = 256, .y = (384 + 32)}, 8, 32,
                                  EnemyAnimationMode::Loop);
    animations[32].SetSquareSheet({.x = 256, .y = (384 + 64)}, 8, 32,
                                  EnemyAnimationMode::Loop);
    animations[33].SetDirectionalSheet({.x = 0, .y = 0}, 32);
    animations[34].SetDirectionalSheet({.x = 0, .y = 32}, 32);
    animations[35].SetDirectionalSheet({.x = 0, .y = 64}, 32);
    animations[36].SetDirectionalSheet({.x = 0, .y = 96}, 32);
    animations[37].SetDirectionalSheet({.x = 0, .y = 128}, 32);
    animations[38].size = {.x = 40, .y = 56};
    animations[38].n = 1;
    animations[38].mode = EnemyAnimationMode::Loop;
    animations[38].ptn[0] = Rect::FromLtwh(512, 0, 40, 56);
    animations[39].size = {.x = 72, .y = 56};
    animations[39].n = 2;
    animations[39].mode = EnemyAnimationMode::Loop;
    animations[39].ptn[0] = {0, 424, 72, 480};
    animations[39].ptn[1] = {72, 424, (72 * 2), 480};
    animations[40].size = {.x = 72, .y = 56};
    animations[40].n = 1;
    animations[40].mode = EnemyAnimationMode::Loop;
    animations[40].ptn[0] = {(72 * 2), 424, (72 * 3), 480};
    animations[41].size = {.x = 40, .y = 64};
    animations[41].n = 1;
    animations[41].mode = EnemyAnimationMode::Loop;
    animations[41].ptn[0] = Rect::FromLtwh(512, 56, 40, 56);
    animations[42].size = {.x = 24, .y = 24};
    animations[42].n = 4;
    animations[42].mode = EnemyAnimationMode::Loop;
    animations[42].ptn[0] = Rect::FromLtwh(552, 0, 24, 24);
    animations[42].ptn[1] = Rect::FromLtwh(552, 24, 24, 24);
    animations[42].ptn[2] = Rect::FromLtwh(552, 0, 24, 24);
    animations[42].ptn[3] = Rect::FromLtwh(552, 48, 24, 24);
    break;
  case StageId::Stage1:
    animations[0].size = {.x = 72, .y = 56};
    animations[0].n = 2;
    animations[0].mode = EnemyAnimationMode::Loop;
    animations[0].ptn[0] = {0, 0, 72, 56};
    animations[0].ptn[1] = {72, 0, (72 * 2), 56};
    animations[1].SetDirectionalSheet({.x = 0, .y = (56 + 0)}, 32);
    animations[2].SetDirectionalSheet({.x = 0, .y = (56 + 32)}, 32);
    animations[3].SetDirectionalSheet({.x = 0, .y = (56 + 64)}, 32);
    animations[4].SetDirectionalSheet({.x = 0, .y = (56 + 96)}, 32);
    animations[5].size = {.x = 72, .y = 64};
    animations[5].n = 1;
    animations[5].mode = EnemyAnimationMode::Loop;
    animations[5].ptn[0] = {0, 184, 72, 248};
    animations[6].size = {.x = 72, .y = 56};
    animations[6].n = 2;
    animations[6].mode = EnemyAnimationMode::Loop;
    animations[6].ptn[0] = {(72 * 2), 0, (72 * 3), 56};
    animations[6].ptn[1] = {(72 * 3), 0, (72 * 4), 56};
    animations[7].size = {.x = 72, .y = 64};
    animations[7].n = 1;
    animations[7].mode = EnemyAnimationMode::Loop;
    animations[7].ptn[0] = {72, 184, (72 * 2), 248};
    break;
  case StageId::Stage2:
    animations[0].SetDirectionalSheet({.x = 0, .y = 0}, 32);
    animations[1].SetDirectionalSheet({.x = 0, .y = 32}, 32);
    animations[2].SetDirectionalSheet({.x = 0, .y = 64}, 32);
    animations[3].SetDirectionalSheet({.x = 0, .y = 96}, 32);
    animations[4].SetDirectionalSheet({.x = 0, .y = 128}, 32);
    animations[5].size = {.x = 112, .y = 48};
    animations[5].n = 1;
    animations[5].mode = EnemyAnimationMode::Loop;
    animations[5].ptn[0] = {0, 160, 112, 208};
    animations[6].size = {.x = 64, .y = 48};
    animations[6].n = 1;
    animations[6].mode = EnemyAnimationMode::Loop;
    animations[6].ptn[0] = {112, 160, 176, 208};
    animations[7].size = {.x = 64, .y = 64};
    animations[7].n = 1;
    animations[7].mode = EnemyAnimationMode::Loop;
    animations[7].ptn[0] = {0, 208, 64, 272};
    animations[8].size = {.x = 112, .y = 48};
    animations[8].n = 1;
    animations[8].mode = EnemyAnimationMode::Loop;
    animations[8].ptn[0] = {176, 160, 288, 208};
    animations[9].size = {.x = 64, .y = 48};
    animations[9].n = 1;
    animations[9].mode = EnemyAnimationMode::Loop;
    animations[9].ptn[0] = {288, 160, 352, 208};
    animations[10].size = {.x = 112, .y = 48};
    animations[10].n = 1;
    animations[10].mode = EnemyAnimationMode::Loop;
    animations[10].ptn[0] = {176, (160 + 48), 288, (208 + 48)};
    animations[11].size = {.x = 64, .y = 48};
    animations[11].n = 1;
    animations[11].mode = EnemyAnimationMode::Loop;
    animations[11].ptn[0] = {288, (160 + 48), 352, (208 + 48)};
    animations[12].size = {.x = 64, .y = 64};
    animations[12].n = 1;
    animations[12].mode = EnemyAnimationMode::Loop;
    animations[12].ptn[0] = {64, 208, 128, 272};
    SetAnimeRect(animations[14], 0, 288, 159, 479);
    SetAnimeRect(animations[15], 160, 384, 271, 479);
    SetAnimeRect(animations[16], 272, 368, 390, 478);
    SetAnimeRect(animations[17], 400, 368, 496, 431);
    SetAnimeRect(animations[18], 400, 160, 558, 359);
    SetAnimeRect(animations[19], 528, 48, 639, 160);
    SetAnimeRect(animations[20], 560, 160, 639, 270);
    SetAnimeRect(animations[21], 576, 320, 639, 399);
    break;
  case StageId::Stage3:
    animations[0].size = {.x = 56, .y = 56};
    animations[0].n = 16;
    animations[0].mode = EnemyAnimationMode::Directional;
    for (i = 0; i < 8; i++) {
      animations[0].ptn[i] = Rect::FromLtwh(i * 56, 0, 56, 56);
    }
    for (i = 0; i < 8; i++) {
      animations[0].ptn[i + 8] = Rect::FromLtwh(i * 56, 56, 56, 56);
    }
    animations[1].SetDirectionalSheet({.x = 0, .y = 112}, 32);
    animations[2].SetDirectionalSheet({.x = 0, .y = 144}, 32);
    animations[3].SetDirectionalSheet({.x = 0, .y = 176}, 32);
    animations[4].size = {.x = 48, .y = 16};
    animations[4].n = 2;
    animations[4].mode = EnemyAnimationMode::Loop;
    animations[4].ptn[0] = Rect::FromLtwh(592, 0, 48, 16);
    animations[4].ptn[1] = Rect::FromLtwh(592, 16, 48, 16);
    animations[5].size = {.x = 48, .y = 16};
    animations[5].n = 2;
    animations[5].mode = EnemyAnimationMode::Loop;
    animations[5].ptn[0] = Rect::FromLtwh(592, 32, 48, 16);
    animations[5].ptn[1] = Rect::FromLtwh(592, 48, 48, 16);
    animations[6].size = {.x = 11 * 16, .y = (5 * 16) + 8};
    animations[6].n = 1;
    animations[6].mode = EnemyAnimationMode::Loop;
    animations[6].ptn[0] = Rect::FromLtwh(464, 392, (11 * 16), ((5 * 16) + 8));
    animations[7].SetDirectionalSheet({.x = 0, .y = 208}, 32);
    animations[8].SetDirectionalSheet({.x = 0, .y = 240}, 40);
    animations[10].size = {.x = 196, .y = 100};
    animations[10].n = 1;
    animations[10].mode = EnemyAnimationMode::Loop;
    animations[10].ptn[0] = {444, 292, 640, 392};
    animations[9].size = {.x = 128, .y = 76};
    animations[9].n = 1;
    animations[9].mode = EnemyAnimationMode::Loop;
    animations[9].ptn[0] = {512, 164, 640, 240};
    break;
  case StageId::Stage4:
    animations[0].SetDirectionalSheet({.x = 0, .y = 0}, 32);
    animations[1].SetDirectionalSheet({.x = 0, .y = 32}, 32);
    animations[2].SetDirectionalSheet({.x = 0, .y = 64}, 32);
    animations[3].SetSquareSheet({.x = 0, .y = 96}, 2, 32,
                                 EnemyAnimationMode::Loop);
    animations[4].SetDirectionalSheet({.x = 64, .y = 96}, 24);
    animations[5].SetDirectionalSheet({.x = 0, .y = 128}, 32);
    animations[6].size = {.x = (640 - 304), .y = (480 - 296)};
    animations[6].n = 1;
    animations[6].mode = EnemyAnimationMode::Loop;
    animations[6].ptn[0] = {304, 296, 640, 480};
    animations[7].size = {.x = (640 - 304 - 32), .y = (480 - 296)};
    animations[7].n = 1;
    animations[7].mode = EnemyAnimationMode::Loop;
    animations[7].ptn[0] = {0, 296, 304, 480};
    break;
  case StageId::Stage5:
    animations[0].SetDirectionalSheet({.x = 0, .y = 0}, 32);
    animations[1].SetDirectionalSheet({.x = 0, .y = 32}, 32);
    animations[2].SetDirectionalSheet({.x = 0, .y = 64}, 32);
    animations[3].SetDirectionalSheet({.x = 0, .y = 96}, 32);
    animations[4].SetDirectionalSheet({.x = 0, .y = 128}, 32);
    animations[5].SetSquareSheet({.x = 512, .y = 0}, 4, 32,
                                 EnemyAnimationMode::Loop);
    animations[6].SetSquareSheet({.x = 512, .y = 64}, 4, 32,
                                 EnemyAnimationMode::Loop);
    animations[7].size = {.x = 24, .y = 24};
    animations[7].n = 4;
    animations[7].mode = EnemyAnimationMode::Loop;
    animations[7].ptn[0] = Rect::FromLtwh(592, (96 + 0), 24, 24);
    animations[7].ptn[1] = Rect::FromLtwh(592, (96 + 24), 24, 24);
    animations[7].ptn[2] = Rect::FromLtwh(592, (96 + 0), 24, 24);
    animations[7].ptn[3] = Rect::FromLtwh(592, (96 + 48), 24, 24);
    animations[8].SetSheet({.x = 512, .y = 96}, 1, {.x = 80, .y = 72},
                           EnemyAnimationMode::Loop);
    animations[9].SetSheet({.x = 304, .y = 256}, 1, {.x = 336, .y = 224},
                           EnemyAnimationMode::Loop);
    break;
  case StageId::Stage6:
    animations[0].size = {.x = 56, .y = 72};
    animations[0].n = 6;
    animations[0].mode = EnemyAnimationMode::StopAtEnd;
    for (i = 0; i < 6; i++) {
      animations[0].ptn[i] = Rect::FromLtwh((56 * i), 72, 56, 72);
    }
    animations[1].size = {.x = 56, .y = 72};
    animations[1].n = 6;
    animations[1].mode = EnemyAnimationMode::StopAtEnd;
    for (i = 0; i < 6; i++) {
      animations[1].ptn[i] = Rect::FromLtwh((56 * (5 - i)), 72, 56, 72);
    }
    animations[2].size = {.x = 56, .y = 72};
    animations[2].n = 4;
    animations[2].mode = EnemyAnimationMode::Loop;
    animations[2].ptn[0] = Rect::FromLtwh((56 * 6), 72, 56, 72);
    animations[2].ptn[1] = Rect::FromLtwh((56 * 7), 72, 56, 72);
    animations[2].ptn[2] = Rect::FromLtwh((56 * 6), 72, 56, 72);
    animations[2].ptn[3] = Rect::FromLtwh((56 * 8), 72, 56, 72);
    animations[3].size = {.x = 56, .y = 72};
    animations[3].n = 10;
    animations[3].mode = EnemyAnimationMode::StopAtEnd;
    for (i = 0; i < 9; i++) {
      animations[3].ptn[i] = Rect::FromLtwh((56 * i), 0, 56, 72);
    }
    animations[3].ptn[9] = Rect::FromLtwh((56 * 5), 72, 56, 72);
    SetAnimeRect(animations[4], 432, 272, 632, 464);
    animations[5].size = {.x = 56, .y = 72};
    animations[5].n = 11;
    animations[5].mode = EnemyAnimationMode::StopAtEnd;
    for (i = 0; i < 6; i++) {
      animations[5].ptn[i] = Rect::FromLtwh((56 * i), 72, 56, 72);
    }
    for (i = 0; i < 5; i++) {
      animations[5].ptn[i + 6] = Rect::FromLtwh((56 * (4 - i)), 72, 56, 72);
    }
    animations[6].size = {.x = 33, .y = 32};
    animations[6].n = 10;
    animations[6].mode = EnemyAnimationMode::StopAtEnd;
    for (i = 0; i < 6; i++) {
      animations[6].ptn[i] = Rect::FromLtwh((32 * i), 416, 32, 32);
    }
    for (i = 0; i < 4; i++) {
      animations[6].ptn[i + 6] = Rect::FromLtwh((32 * i), 448, 32, 32);
    }
    animations[7].size = {.x = 33, .y = 32};
    animations[7].n = 10;
    animations[7].mode = EnemyAnimationMode::StopAtEnd;
    for (i = 0; i < 4; i++) {
      animations[7].ptn[i] = Rect::FromLtwh((32 * (3 - i)), 448, 32, 32);
    }
    for (i = 0; i < 6; i++) {
      animations[7].ptn[i + 4] = Rect::FromLtwh((32 * (5 - i)), 416, 32, 32);
    }
    animations[8].SetSheet({.x = 0, .y = 368}, 1, {.x = 48, .y = 48},
                           EnemyAnimationMode::Loop);
    break;
  }
}

} // namespace anime_data
