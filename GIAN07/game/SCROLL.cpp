/*                                                                           */
/*   SCROLL.cpp   スクロール処理                                             */
/*                                                                           */
/*                                                                           */

#include "game/SCROLL.h"
#include "ecl/SCL.h" // ＳＣＬ定義ファイル
#include "ecl/scl_executor.h"
#include "game/CONFIG.h"
#include "game/DEMOPLAY.h"
#include "game/GIAN.h"
#include "game/LEVEL.h"
#include "game/bgm.h"
#include "game/cast.h"
#include "game/debug.h"
#include "game/endian.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"
#include "ui/WindowSys.h"

// マップデータ保存用ヘッダ //
typedef struct tagScrollSaveHeader {
  U32LE Address;    // このデータの開始アドレス
  U32LE ScrollWait; // このレイヤーのディレイ
  U32LE Length;     // このレイヤーの長さ
} ScrollSaveHeader;

// ScrollInfo, SclInfo → scroll_manager.cpp の Scroller に移動
PIXEL_LTRB rcMapChip[1200]; // マップパーツＩＤに対する矩形

static void enemy_set(void);       // 敵をセットする
static void InitMapChipRect(void); // スクロールに関する情報の初期化を行う

static PBGMAP *ScNextLine(PBGMAP *p);   // 次の行にＧＯ！！
static PBGMAP *ScBeforeLine(PBGMAP *p); // 前の行にＧＯ！！

static void ScrollCmdDummy(void);       // 特殊スクロール無し
static void ScrollCmdStg2Boss(void);    // ２面ボスのスクロール
static void ScrollCmdRasterOpen(void);  // ラスタースクロールオープン
static void ScrollCmdRasterClose(void); // ラスタースクロールクローズ
static void ScrollCmdStg3Boss(void);    // ゲイツ雲

static void ScrollCmdStg6Cube(void);   // ６面の３Ｄキューう゛
static void ScrollCmdStg6RndEcl(void); // ６面のランダムＥＣＬ列
static void ScrollCmdStg6Raster(void); // ６面ラスター

static void ScrollCmdStg3Star(void); // ３面高速星

static void Stg3BossMapDraw(void); // ゲイツ雲描画

static void ScrollCmdStg4Rock(void); // ４面岩

// デバッグ用マクロ //

// 背景を動かす(１フレーム分) //
void ScrollMove(void) {
  int i;

  enemy_set();        // 敵をセット
  ScrollInfo.ExCmd(); // 特殊スクロール発動!!

  // 振動エフェクトを動作させる(これは、特殊スクロールとは別物) //
  if (ScrollInfo.IsQuake)
    ScrollInfo.IsQuake += 2;

  // 標準のスクロールスピードだけカウンタを進める //
  if (ScrollInfo.DataHead == nullptr) {
    return;
  }
  if (ScrollInfo.Count >= ScrollInfo.InfEnd)
    return;

  // スクロールしない場合は、リターンする //
  if (ScrollInfo.ScrollSpeed == 0)
    return;

  ScrollInfo.Count += ScrollInfo.ScrollSpeed;

  if (ScrollInfo.ScrollSpeed > 0) {
    // 通常のスクロール //
    for (i = 0; i < ScrollInfo.NumLayer; i++) {
      ScrollInfo.LayerCount[i] += ScrollInfo.ScrollSpeed;
      while (ScrollInfo.LayerCount[i] >= ScrollInfo.LayerWait[i]) {
        ScrollInfo.LayerCount[i] -= ScrollInfo.LayerWait[i];
        ScrollInfo.LayerDy[i] = (ScrollInfo.LayerDy[i] + 1) % 16; //& 0x0f;
        if (ScrollInfo.LayerDy[i] == 0)
          ScrollInfo.LayerPtr[i] = ScNextLine(ScrollInfo.LayerPtr[i]);
      }
    }
  } else {
    // 逆方向のスクロール //
    for (i = 0; i < ScrollInfo.NumLayer; i++) {
      ScrollInfo.LayerCount[i] += ScrollInfo.ScrollSpeed;
      while (ScrollInfo.LayerCount[i] < 0) {
        if (ScrollInfo.LayerDy[i] == 0)
          ScrollInfo.LayerPtr[i] = ScBeforeLine(ScrollInfo.LayerPtr[i]);
        ScrollInfo.LayerCount[i] += ScrollInfo.LayerWait[i];
        ScrollInfo.LayerDy[i] = (ScrollInfo.LayerDy[i] + 15) % 16; //& 0x0f;
      }
    }
  }
}

static PBGMAP *ScNextLine(PBGMAP *p) {
  int i;

  for (i = 0; i < MAP_WIDTH;) {
    if (*p != MAPDATA_NONE)
      p++, i++;
    else {
      i = i + (*(p + 1));
      p = p + 2;
    }
  }

  return p;
}

static PBGMAP *ScBeforeLine(PBGMAP *p) {
  int i;

  for (i = 0; i < MAP_WIDTH;) {
    if (*(p - 2) != MAPDATA_NONE)
      p--, i++;
    else {
      i = i + (*(p - 1));
      p = p - 2;
    }
  }

  return p;
}

// p:SCL_ENEMY以降の敵配置データ //
// SclKeyWaitCount → scroll_manager.cpp の Scroller に移動

static void enemy_set(void) { SclExecute(SCL_Now); }

/*
static void ExDraw(void)
{
        int			x,y;
        int			dx,dy;
        int			infx,infy;
        int			i,j;

        int			ox,oy;

        static BYTE		deg = 0;
        static int		len = 0;
        static BYTE		count = 0;
        static char		flag = 1;

        ox = cosl(deg,len*flag)+320;
        oy = sinl(deg,len*flag)+240;

        count++;

        if(count==0)	flag *= -1;

        if(count>60)	len = (len+4)%64;
        else			deg+=4;

        GrpGeom->Lock();
        GrpGeom->SetColor({ 0, 3, 0 });

        dx = cosl(deg+64,64);		infx = cosl(deg,800);
        dy = sinl(deg+64,64);		infy = sinl(deg,800);
        for(i=-10;i<=10;i++){
                x = ox + dx*i;
                y = oy + dy*i;
                GrpGeom->DrawLine((x - infx), (y - infy), (x + infx), (y +
infy));
        }

        dx = cosl(deg,64);		infx = cosl(deg+64,800);
        dy = sinl(deg,64);		infy = sinl(deg+64,800);
        for(i=-10;i<=10;i++){
                x = ox + dx*i;
                y = oy + dy*i;
                GrpGeom->DrawLine((x - infx), (y - infy), (x + infx), (y +
infy));
        }

        GrpGeom->Unlock();
}
*/

// 背景を描画する //
void ScrollDraw(void) {
  PBGMAP *p;
  int i, j, k, x, y;
  int dx = 0;   // 振動用
  int RasterDx; // ラスター用

  if (ScrollInfo.DataHead == nullptr) {
    return;
  }

  // 例外処理：ワイドショット用ボム発動中 //
  // if(Viv.bomb_time && Viv.weapon==0){
  //	return;
  //}

  // 特殊描画モード //
  if (ScrollInfo.ExCmd == ScrollCmdStg3Boss) {
    Stg3BossMapDraw();
    return;
  } else if (ScrollInfo.ExCmd == ScrollCmdStg6Cube) {
    Draw3DCube();
    return;
  } else if (ScrollInfo.ExCmd == ScrollCmdStg6RndEcl) {
    DrawEffectFakeECL();
    return;
  } else if (ScrollInfo.ExCmd == ScrollCmdStg6Raster) {
    DrawStg6Raster();
    return;
  } else if (ScrollInfo.ExCmd == ScrollCmdStg3Star) {
    DrawStg3Star();
    return;
  }

  // 振動エフェクト用 //
  // dx = sinl(ScrollInfo.IsQuake*4,2);
  // if(ScrollInfo.IsQuake) dx =
  // sinl(ScrollInfo.IsQuake*8+i*6,(256-ScrollInfo.IsQuake)>>2);	//4
  if (ScrollInfo.IsQuake)
    dx = sinl(ScrollInfo.IsQuake * 16, (256 - ScrollInfo.IsQuake) >> 5); // 4

  // 全てのレイヤーの表示 //
  for (k = 0; k < ScrollInfo.NumLayer; k++) {
    p = ScrollInfo.LayerPtr[k];
    for (i = 29; i >= -1; i--) {
      RasterDx = (k == 0) ? ScrollInfo.RasterDx[i + 1] : 0;
      for (j = 0; j < MAP_WIDTH;) {
        // 通常の描画 //
        if (*p != MAPDATA_NONE) {
          x = (j << 4) + X_MIN + dx + RasterDx;
          y = (i << 4) + ScrollInfo.LayerDy[k];
          const auto &src = rcMapChip[*p];
          GrpSurface_Blit({x, y}, SURFACE_ID::MAPCHIP, src);
          p++, j++;
        }
        // 何もない場合 //
        else {
          j = j + (*(p + 1));
          p = p + 2;
        }
      }
    }
  }

  if (ScrollInfo.ExCmd == ScrollCmdStg4Rock) {
    DrawStg4Rock();
  }
  /*
          if(ScrollInfo.ExCmd==ScrollCmdStg2Boss){
                  ExDraw();
          }
  */
}

// スクロールスピードを変更する(引数:(1)スクロール速度) //
void ScrollSpeed(int speed) {
  // if(speed<0) speed = 0;		// 逆方向スクロール禁止！！

  ScrollInfo.ScrollSpeed = speed;
}

// ＳＣＬ用コマンド実行関数(引数:(1)スクロールコマンド) //
void ScrollCommand(uint8_t cmd) {
  switch (cmd) {
  case (SCMD_QUAKE): // 振動エフェクト
    ScrollInfo.IsQuake = 2;
    break;

  case (SCMD_STG2BOSS): // ２面ボス
    ScrollInfo.ExCmd = ScrollCmdStg2Boss;
    ScrollInfo.ExCount = 0;
    break;

  case (SCMD_STG3BOSS): // ３面ボス
    ScrollInfo.ExCmd = ScrollCmdStg3Boss;
    ScrollInfo.ExCount = 0;
    // InitStg3Cloud();
    break;

  case (SCMD_STG3RESET):
    ScrollInfo.ExCmd = ScrollCmdDummy;
    ScrollInfo.ExCount = 0;
    break;

  case (SCMD_STG6CUBE):
    ScrollInfo.ExCmd = ScrollCmdStg6Cube;
    ScrollInfo.ExCount = 0;
    Init3DCube();
    break;

  case (SCMD_STG6RNDECL):
    ScrollInfo.ExCmd = ScrollCmdStg6RndEcl;
    ScrollInfo.ExCount = 0;
    InitEffectFakeECL();
    break;

  case (SCMD_STG4ROCK):
    ScrollInfo.ExCmd = ScrollCmdStg4Rock;
    ScrollInfo.ExCount = 0;
    InitStg4Rock();
    break;

  case (SCMD_STG4LEAVE):
    if (ScrollInfo.ExCmd != ScrollCmdStg4Rock)
      break;
    SendCmdStg4Rock(STG4ROCK_LEAVE, 0);
    break;

  case (SCMD_STG6RASTER):
    ScrollInfo.ExCmd = ScrollCmdStg6Raster;
    ScrollInfo.ExCount = 0;
    InitStg6Raster();
    break;

  case (SCMD_STG3STAR):
    ScrollInfo.ExCmd = ScrollCmdStg3Star;
    ScrollInfo.ExCount = 0;
    InitStg3Star();
    ScreenEffectSet(SCNEFC_WHITEIN);
    break;

  case (SCMD_RASTER_ON): // ラスタースクロール開始
    ScrollInfo.ExCmd = ScrollCmdRasterOpen;
    ScrollInfo.RasterDeg = 0;
    ScrollInfo.RasterWidth = 0;
    break;

  case (SCMD_RASTER_OFF): // ラスタースクロール終了
    ScrollInfo.ExCmd = ScrollCmdRasterClose;
    // ScrollInfo.RasterDeg   = 0;
    // ScrollInfo.RasterWidth = 0;
    break;
  }
}

// 特殊スクロール無し //
static void ScrollCmdDummy(void) {
  // 何もしないよぉ... //
}

// ２面ボスのスクロール //
static void ScrollCmdStg2Boss(void) {
  /*
          SSP	-810	TR	10			SSP	-630	TR
     10 SSP	-450	TR	10			SSP	-270	TR
     10 SSP	-180	TR	10			SSP	-90
     TR	10 SSP	0		TR	10			SSP	90
     TR	10 SSP	180		TR	10			SSP	450
     TR	10 SSP	630		TR	10			SSP	810
     TR	300
  */
  // 特殊タイマーにより分岐を行う //
  switch (ScrollInfo.ExCount) {
  // 正方向->逆方向 //
  case (0):
    ScrollSpeed(1512);
    break;
  case (20):
    ScrollSpeed(1200);
    break;
  case (40):
    ScrollSpeed(900);
    break;
  case (60):
    ScrollSpeed(600);
    break;
  case (80):
    ScrollSpeed(300);
    break;
  case (100):
    ScrollSpeed(150);
    break;
  // case(120):	ScrollSpeed(0);			break;
  case (140):
    ScrollSpeed(-150);
    break;
  case (160):
    ScrollSpeed(-300);
    break;
  case (180):
    ScrollSpeed(-600);
    break;
  case (200):
    ScrollSpeed(-900);
    break;
  case (220):
    ScrollSpeed(-1200);
    break;
  case (240):
    ScrollSpeed(-1512);
    break;

  // 逆方向->正方向 //
  case (440):
    ScrollSpeed(-1512);
    break;
  case (460):
    ScrollSpeed(-1200);
    break;
  case (480):
    ScrollSpeed(-900);
    break;
  case (500):
    ScrollSpeed(-600);
    break;
  case (520):
    ScrollSpeed(-300);
    break;
  case (540):
    ScrollSpeed(-150);
    break;
  // case(560):	ScrollSpeed(0);			break;
  case (580):
    ScrollSpeed(150);
    break;
  case (600):
    ScrollSpeed(300);
    break;
  case (620):
    ScrollSpeed(600);
    break;
  case (640):
    ScrollSpeed(900);
    break;
  case (660):
    ScrollSpeed(1200);
    break;
  case (680):
    ScrollSpeed(1512);
    break;
  }

  ScrollInfo.ExCount = (ScrollInfo.ExCount + 1) % 880;
}

// ラスタースクロールオープン //
static void ScrollCmdRasterOpen(void) {
  int i, j;

  // ちょっと重いかもね... //
  for (i = j = 0; i < 31; i++, j += 16) {
    ScrollInfo.RasterDx[i] = Cast::down<int8_t>(
        sinl((ScrollInfo.RasterDeg + j), ScrollInfo.RasterWidth));
  }

  ScrollInfo.RasterDeg += 2;

  if (ScrollInfo.RasterWidth < 2)
    ScrollInfo.RasterWidth++;
}

// ラスタースクロールクローズ //
static void ScrollCmdRasterClose(void) {
  int i, j;

  // ちょっと重いかもね... //
  for (i = j = 0; i < 31; i++, j += 2) {
    ScrollInfo.RasterDx[i] = Cast::down<int8_t>(
        sinl((ScrollInfo.RasterDeg + j), ScrollInfo.RasterWidth));
  }

  ScrollInfo.RasterDeg += 8;

  ScrollInfo.RasterWidth--;

  if (ScrollInfo.RasterWidth == 0) {
    ScrollInfo.ExCmd = ScrollCmdDummy;
  }
}

// ゲイツ雲 //
static void ScrollCmdStg3Boss(void) {
  ScrollInfo.ExCount = (ScrollInfo.ExCount + 200) % 208;
  //	MoveStg3Cloud();
}

// ゲイツ雲描画 //
static void Stg3BossMapDraw(void) {
  int x, y;

  x = X_MIN;
  y = Y_MIN - ScrollInfo.ExCount;

  for (; y < Y_MAX; y += 208) {
    constexpr PIXEL_LTRB src = {0, 272, (640 - 256), (272 + 208)};
    GrpSurface_Blit({x, y}, SURFACE_ID::MAPCHIP, src);
  }

  //	DrawStg3Cloud();
}

// ６面の３Ｄキューう゛ //
static void ScrollCmdStg6Cube(void) { Move3DCube(); }

// ６面のランダムＥＣＬ列 //
static void ScrollCmdStg6RndEcl(void) { MoveEffectFakeECL(); }

// ６面ラスター //
static void ScrollCmdStg6Raster(void) { MoveStg6Raster(); }

// ４面岩 //
static void ScrollCmdStg4Rock(void) { MoveStg4Rock(); }

// ３面高速星 //
static void ScrollCmdStg3Star(void) {
  ScrollInfo.ExCount++;

  if (ScrollInfo.ExCount == 32) {
    ScreenEffectSet(SCNEFC_WHITEOUT);
  }

  MoveStg3Star();
}

// マップデータをロードする(BMP含む) //
bool ScrollInit(void) {
  ScrollSaveHeader *LayerInfo;
  int i;
  static bool bInitialized = false;

  SclInfo.MsgFlag = false;
  SclInfo.ReturnFlag = false;

  if (!bInitialized) {
    InitMapChipRect();
    bInitialized = true;
  }

  /*
          // 読み込みの準備 //
          auto in = FilStartR("GIAN_MAP.DAT");
          if(!in) {
                  return false;
          }

          // もしすでにロードが行われていた場合、メモリを解放する //
          if(ScrollInfo.DataHead != nullptr) {
                  LocalFree(ScrollInfo.DataHead);
                  ScrollInfo.DataHead = nullptr;
          }
          // そうでなければ、スクロールに関する初期化を行う //
          else if(!InitScrollInfo()) {
                  return false;
          }

          // 解凍を行う(後で、ステージを考慮したものに変更すること) //
          ScrollInfo.DataHead = in->MemExpand(0);
          if(!ScrollInfo.DataHead) {
                  return false;
          }
  */
  // 標準のスクロールスピード(マップエディタと同じ) //
  auto *head = ScrollInfo.DataHead.get();
  ScrollInfo.ScrollSpeed = TIME_PER_FRAME; // 標準のスクロール速度
  ScrollInfo.NumLayer = U32LEAt(head);     // レイヤーの数
  ScrollInfo.Count = 0;                    // スクロール用カウンタ
  ScrollInfo.InfStart = 0;                 // 無限ループ開始時刻
  ScrollInfo.State = SST_NORMAL;           // 状態(デフォルトの標準で..)
  ScrollInfo.IsQuake = 0;                  // 振動中ではない
  ScrollInfo.ExCmd = ScrollCmdDummy;       // 特殊コマンド
  ScrollInfo.ExCount = 0;
  ScrollInfo.RasterWidth = 0;
  ScrollInfo.RasterDeg = 0;

  // ラスタースクロールの初期化 //
  for (auto &it : ScrollInfo.RasterDx) {
    it = 0;
  }

  // レイヤー情報のロード(長さからループ用変数を調整すること) //
  LayerInfo = reinterpret_cast<ScrollSaveHeader *>(head + sizeof(U32LE));
  for (i = 0; i < ScrollInfo.NumLayer; i++) {
    auto *p = reinterpret_cast<PBGMAP *>(head + LayerInfo[i].Address);
    ScrollInfo.LayerHead[i] = p;                       // 先頭
    ScrollInfo.LayerPtr[i] = p;                        // 現在
    ScrollInfo.LayerWait[i] = LayerInfo[i].ScrollWait; // レイヤーの重み
    ScrollInfo.LayerCount[i] = 0;
    ScrollInfo.LayerDy[i] = 0;
  }

  // 無限ループ終了時刻 //
  ScrollInfo.InfEnd =
      16 * (LayerInfo[i - 1].Length - 1280 / 16) * LayerInfo[i - 1].ScrollWait;

  return true;
}

// スクロールに関する情報の初期化を行う //
static void InitMapChipRect(void) {
  int i, x, y;

  // マップチップ用矩形の準備 //
  for (i = 0; i < 1200; i++) {
    x = (i % (640 / 16)) << 4; // マップエディタと同様の演算式
    y = (i / (640 / 16)) << 4; // マップエディタと同様の演算式
    rcMapChip[i] = {x, y, (x + 16), (y + 16)};
  }
}
