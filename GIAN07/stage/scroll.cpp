/*                                                                           */
/*   SCROLL.cpp   スクロール処理                                             */
/*                                                                           */
/*                                                                           */

#include "scroll.h"

#include "config.h"
#include "demo_play.h"
#include "game/bgm.h"
#include "game/cast.h"
#include "game/debug.h"
#include "game/endian.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "gian.h"
#include "level.h"
#include "platform/graphics_backend.h"
#include "scene.h" // ＳＣＬ定義ファイル
#include "ui_manager.h"
#include "window_sys.h"
#include <utility>

// マップデータ保存用ヘッダ //
struct ScrollSaveHeader {
  U32LE Address;    // このデータの開始アドレス
  U32LE ScrollWait; // このレイヤーのディレイ
  U32LE Length;     // このレイヤーの長さ
};

// ScrollInfo, SclInfo, Scroller.map_chip_rects[] → scroll_manager.cpp の
// ScrollManager に移動

static void enemy_set();                // 敵をセットする
static void PutEnemy(const uint8_t *p); // p:SCL_ENEMY以降の敵配置データ
static void InitMapChipRect();          // スクロールに関する情報の初期化を行う

static PBGMAP *ScNextLine(PBGMAP *p);   // 次の行にＧＯ！！
static PBGMAP *ScBeforeLine(PBGMAP *p); // 前の行にＧＯ！！

static void ScrollCmdDummy();       // 特殊スクロール無し
static void ScrollCmdStg2Boss();    // ２面ボスのスクロール
static void ScrollCmdRasterOpen();  // ラスタースクロールオープン
static void ScrollCmdRasterClose(); // ラスタースクロールクローズ
static void ScrollCmdStg3Boss();    // ゲイツ雲

static void ScrollCmdStg6Cube();   // ６面の３Ｄキューう゛
static void ScrollCmdStg6RndEcl(); // ６面のランダムＥＣＬ列
static void ScrollCmdStg6Raster(); // ６面ラスター

static void ScrollCmdStg3Star(); // ３面高速星

static void Stg3BossMapDraw(); // ゲイツ雲描画

static void ScrollCmdStg4Rock(); // ４面岩

// デバッグ用マクロ //
static void SCL_DEBUG(std::string_view s) {
#ifdef SCRIPT_TRACE
  DebugLog(s);
#endif
}

// 背景を動かす(１フレーム分) //
void ScrollManager::Move() {
  int i = 0;

  enemy_set();    // 敵をセット
  scroll.ExCmd(); // 特殊スクロール発動!!

  // 振動エフェクトを動作させる(これは、特殊スクロールとは別物) //
  if (scroll.IsQuake != 0U) {
    scroll.IsQuake += 2;
  }

  // 標準のスクロールスピードだけカウンタを進める //
  if (scroll.DataHead == nullptr) {
    return;
  }
  if (scroll.Count >= scroll.InfEnd) {
    return;
  }

  // スクロールしない場合は、リターンする //
  if (scroll.ScrollSpeed == 0) {
    return;
  }

  scroll.Count += scroll.ScrollSpeed;

  if (scroll.ScrollSpeed > 0) {
    // 通常のスクロール //
    for (i = 0; i < scroll.NumLayer; i++) {
      scroll.LayerCount[i] += scroll.ScrollSpeed;
      while (
          std::cmp_greater_equal(scroll.LayerCount[i], scroll.LayerWait[i])) {
        scroll.LayerCount[i] -= scroll.LayerWait[i];
        scroll.LayerDy[i] = (scroll.LayerDy[i] + 1) % 16; //& 0x0f;
        if (scroll.LayerDy[i] == 0) {
          scroll.LayerPtr[i] = ScNextLine(scroll.LayerPtr[i]);
        }
      }
    }
  } else {
    // 逆方向のスクロール //
    for (i = 0; i < scroll.NumLayer; i++) {
      scroll.LayerCount[i] += scroll.ScrollSpeed;
      while (scroll.LayerCount[i] < 0) {
        if (scroll.LayerDy[i] == 0) {
          scroll.LayerPtr[i] = ScBeforeLine(scroll.LayerPtr[i]);
        }
        scroll.LayerCount[i] += scroll.LayerWait[i];
        scroll.LayerDy[i] = (scroll.LayerDy[i] + 15) % 16; //& 0x0f;
      }
    }
  }
}

static PBGMAP *ScNextLine(PBGMAP *p) {
  int i = 0;

  for (i = 0; i < MAP_WIDTH;) {
    if (*p != MAPDATA_NONE) {
      {
        p++, i++;
      }
    } else {
      i = i + (*(p + 1));
      p = p + 2;
    }
  }

  return p;
}

static PBGMAP *ScBeforeLine(PBGMAP *p) {
  int i = 0;

  for (i = 0; i < MAP_WIDTH;) {
    if (*(p - 2) != MAPDATA_NONE) {
      {
        p--, i++;
      }
    } else {
      i = i + (*(p - 1));
      p = p - 2;
    }
  }

  return p;
}

// p:SCL_ENEMY以降の敵配置データ //
// Scroller.key_wait_count → scroll_manager.cpp の Scroller に移動

static void enemy_set() {
  bool bFlag = true;
  //	BIT_DEVICE	*in;				// やばやば...
  bool CtrlFlag = false;

  while (bFlag) {
    const auto *cmd = Enemies.scl_now;
    switch (cmd[0]) {
    case SCL_KEY: // キー入力待ち
      if (((Key_Data & (KEY_TAMA | KEY_RETURN | KEY_BOMB)) != 0) ||
          ++Scroller.key_wait_count >= 180) {
        Enemies.scl_now++;
        Scroller.key_wait_count = 0;
      } else {
        bFlag = false;
      }
      break;

    case SCL_TIME: {
      const auto temp = U32LEAt(&cmd[1]);
      // メッセージウィンドウがオープンしている場合 //
      if (Scroller.scene.MsgFlag) {
        if (((Key_Data & KEY_TAMA) != 0) || ((Key_Data & KEY_RETURN) != 0) ||
            ((Key_Data & KEY_BOMB) != 0)) {
          if (!Scroller.scene.ReturnFlag) {
            GameState.game_count = temp;
            Scroller.scene.ReturnFlag = true;
          }
        } else {
          Scroller.scene.ReturnFlag = false;
        }
      }
      /*
                                      if((Key_Data & KEY_SKIP) &&
         Scroller.scene.MsgFlag)
         GameState.game_count+=(temp-GameState.game_count)/3; else if((Key_Data
         & KEY_RETURN) && !Scroller.scene.ReturnFlag){ GameState.game_count  =
         temp; Scroller.scene.ReturnFlag = true;
                                      }
                                      if(!(Key_Data & KEY_RETURN) &&
         Scroller.scene.ReturnFlag) { Scroller.scene.ReturnFlag = false;
                                      }
      */
      if (temp > GameState.game_count) {
        bFlag = false;
      } else {
        Enemies.scl_now += 5; // cmd(1)+time(4)
      }
      SCL_DEBUG("--- SCL_TIME ---");
    } break;

    case SCL_ENEMY:
      if (Bosses.count == 0) {
        PutEnemy(cmd + 1); // ボス出現中は出て来ちゃダメ
      }
      Enemies.scl_now += 6; // cmd(1)+x(2)+y(2)+id(1)
      SCL_DEBUG("--- SCL_ENEMY ---");
      break;

    case SCL_BOSS: { // ボスをセットする(X(16),Y(16),ID(8))
      const auto x = I16LEAt(&cmd[1 + 0]); // ボス初期Ｘ
      const auto y = I16LEAt(&cmd[1 + 2]); // ボス初期Ｙ
      const auto id = cmd[1 + 2 + 2];      // ボスＩＤ
      Bosses.Set(x, y, id);
      Enemies.scl_now += (1 + 2 + 2 + 1); // cmd+x+y+id
    } break;

    case SCL_BOSSDEAD: // ボスを強制的に破壊する(Level2 命令Only)
      Bosses.KillAll();
      Enemies.scl_now++;
      break;

    case SCL_MWOPEN: // メッセージウィンドウを開く
      if ((ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE) == 0) {
        UI.Msg().Open();
      }
      Scroller.scene.MsgFlag = true;
      Enemies.scl_now++;
      break;

    case SCL_MWCLOSE: // メッセージウィンドウを閉じる
      if ((ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE) == 0) {
        UI.Msg().Close();
      }
      Scroller.scene.MsgFlag = false;
      Enemies.scl_now++;
      break;

    case SCL_MSG: // メッセージを出力する
      // UI.Msg().Cmd(MWCMD_SMALLFONT);
      UI.Msg().Msg(reinterpret_cast<const char *>(cmd + 1));
      Enemies.scl_now += (strlen(reinterpret_cast<const char *>(cmd + 1)) + 2);
      break;

    case SCL_FACE: // 顔を表示する
      UI.Msg().Face(cmd[1]);
      Enemies.scl_now += 2;
      break;

    case SCL_LOADFACE: // 顔グラをロードする(SurfaceID,FileNo)
      LoadFace(cmd[1], cmd[2]);
      Enemies.scl_now += 3;
      break;

    case SCL_NPG: // 新しいページに変更する
      UI.Msg().Cmd(MWCMD_NEWPAGE);
      Enemies.scl_now++;
      break;

    case SCL_END: // カウントも変更させずにリターンする
                  /*
                  UI.Msg().Open();
                  UI.Msg().Cmd(MWCMD_NEWPAGE);
                  UI.Msg().Cmd(MWCMD_LARGEFONT);
                  UI.Msg().Msg("ＳＣＬ完了ですの");
                  SCL_DEBUG("--- SCL_END ---");
                  */
      return;

    case SCL_SSP: // スクロールスピード変更
      Scroller.SetSpeed(I16LEAt(&cmd[1]));
      Enemies.scl_now += 3;
      break;

    case SCL_MUSIC:
      //				if(!(/*DemoplaySaveEnable||*/Demos.load_enable)){
      if (!GameState.is_demoplay) {
        BGM_Stop();
        if (BGM_Switch(cmd[1])) {
          BGM_Play();
          const auto mtitle = BGM_Title();
          if (!mtitle.empty()) {
            Effects.SetMusicTitle(460, mtitle);
          }
        }
      }
      Enemies.scl_now += 2;
      break;

    case SCL_DELENEMY:
      Enemies.InitIndices();
      Enemies.scl_now++;
      break;

    case SCL_EFC:
      switch (cmd[1]) {
      case SEFC_WARN:
        // effect_set(0,0,EFC_WARNBOSS,GameState.game_stage);
        Snd_SEPlay(8, GX_MID, true);
        Effects.SetWarningEffect();
        // StringEffect3(GameState.game_stage);
        break;

      case SEFC_WARNSTOP:
        Snd_SEStop(8);
        break; // Warning 停止
      case SEFC_MUSICFADE:
        BGM_FadeOut(120);
        break; // 曲の停止
      case SEFC_STG2BOSS:
        Scroller.Command(SCMD_STG2BOSS);
        break; // ２面ボスScroll
      case SEFC_RASTERON:
        Scroller.Command(SCMD_RASTER_ON);
        break; // ラスターON
      case SEFC_RASTEROFF:
        Scroller.Command(SCMD_RASTER_OFF);
        break; // ラスターOFF
      case SEFC_STG3BOSS:
        Scroller.Command(SCMD_STG3BOSS);
        break; // ３面ボス雲
      case SEFC_STG3RESET:
        Scroller.Command(SCMD_STG3RESET);
        break; // ３面リセット
      case SEFC_CFADEIN:
        Effects.SetScreenEffect(SCNEFC_CFADEIN);
        break; // ○フェードIn
      case SEFC_CFADEOUT:
        Effects.SetScreenEffect(SCNEFC_CFADEOUT);
        break; // ○フェードOut
      case SEFC_STG6CUBE:
        Scroller.Command(SCMD_STG6CUBE);
        break; // ６面キューブ
      case SEFC_STG6RNDECL:
        Scroller.Command(SCMD_STG6RNDECL);
        break; // ６面ＥＣＬ羅列
      case SEFC_STG4ROCK:
        Scroller.Command(SCMD_STG4ROCK);
        break; // ４面岩
      case SEFC_STG4LEAVE:
        Scroller.Command(SCMD_STG4LEAVE);
        break; // ４面岩画面外へ
      case SEFC_WHITEIN:
        Effects.SetScreenEffect(SCNEFC_WHITEIN);
        break; // ホワイトイン
      case SEFC_WHITEOUT:
        Effects.SetScreenEffect(SCNEFC_WHITEOUT);
        break; // ホワイトアウト
      case SEFC_LOADEX01:
        LoadGraph(GRAPH_ID_EXBOSS1);
        break;
      case SEFC_LOADEX02:
        LoadGraph(GRAPH_ID_EXBOSS2);
        break;
      case SEFC_STG6RASTER:
        Scroller.Command(SCMD_STG6RASTER);
        break; // ６面ラスター
      }
      Enemies.scl_now += 2;
      break;

    case SCL_WAITEX: // 特殊待ち <cmd1>,<opt4>
      switch (cmd[1]) {
      case SWAIT_BOSSHP: // 残りＨＰ
        if (Bosses.GetHPSum() <= U32LEAt(&cmd[2])) {
          break;
        }
        return;
      case SWAIT_BOSSLEFT: // 残りボス数
        if (Bosses.count <= U32LEAt(&cmd[2])) {
          break;
        }
        return;
      }
      Enemies.scl_now += (1 + 1 + 4);
      break;

    case SCL_STAGECLEAR: // ステージクリア
      if (Demos.save_all_enable) {
        Demos.FlushStage();
        GameNextStage();
        return;
      }
      if (Demos.load_all_enable) {
        if (GameState.game_stage < Demos.playback_max_stage) {
          GameNextStage();
        }
        return;
      }
      // ステージクリア処理をここに記述 //
      GameNextStage(); // 本当はエラーチェックが必要!!
      return;

    case SCL_GAMECLEAR:
      if (Demos.save_all_enable) {
        Demos.FlushStage();
        Demos.SaveReplayAll(false);
        // Fall through to normal ending logic
      }
      if (Demos.load_all_enable) {
        return;
      }

      if (GameState.game_stage == STAGE_MAX) {
        GameState.game_stage = 7;
      }
      if (GameState.game_level != GAME_EASY) {
        switch (Players.viv.weapon) {
        case 0:
          ConfigDat.ExtraStgFlags.v |= 1;
          break;
        case 1:
          ConfigDat.ExtraStgFlags.v |= 2;
          break;
        case 2:
          ConfigDat.ExtraStgFlags.v |= 4;
          break;
        }
      }
      ConfigSave();
      Ending.Init();
      return;

    case SCL_EXTRACLEAR:
      if (Demos.save_all_enable) {
        Demos.FlushStage();
        Demos.SaveReplayAll(true);
      }
      if (Demos.load_all_enable) {
        return;
      }

      GameFlow.NameRegistInit(true);
      return;

    case SCL_MAPPALETTE: // マップパーツ用Surface からパレットを
      // 前後にある４０色をマップパーツのパレットにする
      // BitDeapth 判定は、関数側に任せる
      GrpSurface_PaletteApplyToBackend(SURFACE_ID::MAPCHIP);
      Enemies.scl_now++;
      break;

    case SCL_ENEMYPALETTE:
      LoadPaletteFromEnemy(); // BitDeapth 判定は、関数側に任せる
      Enemies.scl_now++;
      break;

    default: // 未実装 or ばぐ
      UI.Msg().Open();
      UI.Msg().Cmd(MWCMD_NEWPAGE);
      UI.Msg().Cmd(MWCMD_LARGEFONT);
      UI.Msg().Msg("バグ発生だにょ");
      SCL_DEBUG("---- SCL !BUG! ---");
      return;
    }
  }

  GameState.game_count++;

  if ((GameState.game_count & 0x3f) == 0) {
    if (GameState.game_stage == GRAPH_ID_EXSTAGE) {
      Ranking.Add(1);
    } else {
      Ranking.Add(1 + (GameState.game_stage / 3));
    }
  }
}

static void PutEnemy(const uint8_t *p) {
  /*
   * [メモ]
   *  p[0-1]:EnemyX  p[2-3]:EnemyY  p[4]:EnemyID
   *  Enemies.ecl_head[0-3]:Num  Enemies.ecl_head[n*4-(n*4+3)]
   * (n>1):StartAddr(ABS)
   */
  EnemyData *e = nullptr;
  short x = 0;
  short y = 0;

  if (Enemies.count + 1 >= ENEMY_MAX) {
    return;
  }

  e = &Enemies.entities[Enemies.indices[Enemies.count++]];

  const uint32_t n = (4 + (p[4] << 2));

  x = I16LEAt(&p[0]); // PixelToWorld(I16LEAt(&p[0]));
  y = I16LEAt(&p[2]); // PixelToWorld(I16LEAt(&p[2]));
  Enemies.InitDataSTD(e, x, y, n);

  /*
          e->x   = I16LEAt(&p[0]);	// PixelToWorld(I16LEAt(&p[0]));
          e->y   = I16LEAt(&p[2]);	// PixelToWorld(I16LEAt(&p[2]));

          e->x = (e->x==X_RNDV) ? GX_RND() : (e->x<<6);
          e->y = (e->y==Y_RNDV) ? GY_RND() : (e->y<<6);
          e->cmd = U32LEAt(&Enemies.ecl_head[n]);

          e->call_addr = e->cmd;

          e->hp       = 0xffffffff;
          e->amp      = 0;
          e->anm_ptn  = 0;
          e->anm_sp   = 0;
          e->anm_c    = 0;
          e->count    = 0;
          e->evscore  = 0;
          e->d        = 64;
          e->flag     = EF_DAMAGE|EF_DRAW|EF_HITSB;

          e->tama_c   = rnd();//&0xff;
          e->t_rep    = 0;			//
     弾の発射間隔(０：自動発射しない) e->g_width  = 0; e->g_height = 0;

          e->item     = 0;

          e->rep_c    = 0;
          e->cmd_c    = 0;
          e->v        = 64;
          e->vd       = 0;
          e->vx       = cosl(e->d,e->v);
          e->vy       = sinl(e->d,e->v);

          e->LLaserRef = 0;

          e->t_cmd.c      = 0;
          e->t_cmd.cmd    = TC_WAY;
          e->t_cmd.d      = 64;
          e->t_cmd.n      = 1;
          e->t_cmd.option = TE_NONE;
          e->t_cmd.type   = T_NORM;
          e->t_cmd.v      = 3;
          e->t_cmd.x      = 0;
          e->t_cmd.y      = 0;

          e->t_cmd.dw     = 16;
          e->t_cmd.ns     = 1;
          e->t_cmd.rep    = 0;
          e->t_cmd.vd     = 0;


          // 変数用レジスタの初期化 //
          e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
          e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;

          // 割り込みベクタの初期化 //
          Enemies.InitInterrupts(e);
  */
}

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
void ScrollManager::Draw() {
  PBGMAP *p = nullptr;
  int i = 0;
  int j = 0;
  int k = 0;
  int x = 0;
  int y = 0;
  int dx = 0;       // 振動用
  int RasterDx = 0; // ラスター用

  if (scroll.DataHead == nullptr) {
    return;
  }

  // 例外処理：ワイドショット用ボム発動中 //
  // if(Players.viv.bomb_time && Players.viv.weapon==0){
  //	return;
  //}

  // 特殊描画モード //
  if (scroll.ExCmd == ScrollCmdStg3Boss) {
    Stg3BossMapDraw();
    return;
  }
  if (scroll.ExCmd == ScrollCmdStg6Cube) {
    Effects.Draw3DCubes();
    return;
  }
  if (scroll.ExCmd == ScrollCmdStg6RndEcl) {
    Effects.DrawFakeECL();
    return;
  }
  if (scroll.ExCmd == ScrollCmdStg6Raster) {
    Effects.DrawStg6Rasters();
    return;
  }
  if (scroll.ExCmd == ScrollCmdStg3Star) {
    Effects.DrawStg3Stars();
    return;
  }

  // 振動エフェクト用 //
  // dx = sinl(scroll.IsQuake*4,2);
  // if(scroll.IsQuake) dx =
  // sinl(scroll.IsQuake*8+i*6,(256-scroll.IsQuake)>>2);	//4
  if (scroll.IsQuake != 0U) {
    dx = sinl(scroll.IsQuake * 16, (256 - scroll.IsQuake) >> 5); // 4
  }

  // 全てのレイヤーの表示 //
  for (k = 0; k < scroll.NumLayer; k++) {
    p = scroll.LayerPtr[k];
    for (i = 29; i >= -1; i--) {
      RasterDx = (k == 0) ? scroll.RasterDx[i + 1] : 0;
      for (j = 0; j < MAP_WIDTH;) {
        // 通常の描画 //
        if (*p != MAPDATA_NONE) {
          x = (j << 4) + X_MIN + dx + RasterDx;
          y = (i << 4) + scroll.LayerDy[k];
          const auto &src = map_chip_rects[*p];
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

  if (scroll.ExCmd == ScrollCmdStg4Rock) {
    Effects.DrawStg4Rocks();
  }
  /*
          if(scroll.ExCmd==ScrollCmdStg2Boss){
                  ExDraw();
          }
  */
}

// スクロールスピードを変更する(引数:(1)スクロール速度) //
void ScrollManager::SetSpeed(int speed) {
  // if(speed<0) speed = 0;		// 逆方向スクロール禁止！！

  scroll.ScrollSpeed = speed;
}

// ＳＣＬ用コマンド実行関数(引数:(1)スクロールコマンド) //
void ScrollManager::Command(uint8_t cmd) {
  switch (cmd) {
  case SCMD_QUAKE: // 振動エフェクト
    scroll.IsQuake = 2;
    break;

  case SCMD_STG2BOSS: // ２面ボス
    scroll.ExCmd = ScrollCmdStg2Boss;
    scroll.ExCount = 0;
    break;

  case SCMD_STG3BOSS: // ３面ボス
    scroll.ExCmd = ScrollCmdStg3Boss;
    scroll.ExCount = 0;
    // InitStg3Cloud();
    break;

  case SCMD_STG3RESET:
    scroll.ExCmd = ScrollCmdDummy;
    scroll.ExCount = 0;
    break;

  case SCMD_STG6CUBE:
    scroll.ExCmd = ScrollCmdStg6Cube;
    scroll.ExCount = 0;
    Effects.Init3DCubes();
    break;

  case SCMD_STG6RNDECL:
    scroll.ExCmd = ScrollCmdStg6RndEcl;
    scroll.ExCount = 0;
    Effects.InitFakeECL();
    break;

  case SCMD_STG4ROCK:
    scroll.ExCmd = ScrollCmdStg4Rock;
    scroll.ExCount = 0;
    Effects.InitStg4Rocks();
    break;

  case SCMD_STG4LEAVE:
    if (scroll.ExCmd != ScrollCmdStg4Rock) {
      break;
    }
    Effects.SendCmdStg4Rocks(STG4ROCK_LEAVE, 0);
    break;

  case SCMD_STG6RASTER:
    scroll.ExCmd = ScrollCmdStg6Raster;
    scroll.ExCount = 0;
    Effects.InitStg6Rasters();
    break;

  case SCMD_STG3STAR:
    scroll.ExCmd = ScrollCmdStg3Star;
    scroll.ExCount = 0;
    Effects.InitStg3Stars();
    Effects.SetScreenEffect(SCNEFC_WHITEIN);
    break;

  case SCMD_RASTER_ON: // ラスタースクロール開始
    scroll.ExCmd = ScrollCmdRasterOpen;
    scroll.RasterDeg = 0;
    scroll.RasterWidth = 0;
    break;

  case SCMD_RASTER_OFF: // ラスタースクロール終了
    scroll.ExCmd = ScrollCmdRasterClose;
    // scroll.RasterDeg   = 0;
    // scroll.RasterWidth = 0;
    break;
  }
}

// 特殊スクロール無し //
static void ScrollCmdDummy() {
  // 何もしないよぉ... //
}

// ２面ボスのスクロール //
static void ScrollCmdStg2Boss() {
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
  switch (Scroller.scroll.ExCount) {
  // 正方向->逆方向 //
  case 0:
    Scroller.SetSpeed(1512);
    break;
  case 20:
    Scroller.SetSpeed(1200);
    break;
  case 40:
    Scroller.SetSpeed(900);
    break;
  case 60:
    Scroller.SetSpeed(600);
    break;
  case 80:
    Scroller.SetSpeed(300);
    break;
  case 100:
    Scroller.SetSpeed(150);
    break;
  // case(120):	Scroller.SetSpeed(0);			break;
  case 140:
    Scroller.SetSpeed(-150);
    break;
  case 160:
    Scroller.SetSpeed(-300);
    break;
  case 180:
    Scroller.SetSpeed(-600);
    break;
  case 200:
    Scroller.SetSpeed(-900);
    break;
  case 220:
    Scroller.SetSpeed(-1200);
    break;
  case 240:
    Scroller.SetSpeed(-1512);
    break;

  // 逆方向->正方向 //
  case 440:
    Scroller.SetSpeed(-1512);
    break;
  case 460:
    Scroller.SetSpeed(-1200);
    break;
  case 480:
    Scroller.SetSpeed(-900);
    break;
  case 500:
    Scroller.SetSpeed(-600);
    break;
  case 520:
    Scroller.SetSpeed(-300);
    break;
  case 540:
    Scroller.SetSpeed(-150);
    break;
  // case(560):	Scroller.SetSpeed(0);			break;
  case 580:
    Scroller.SetSpeed(150);
    break;
  case 600:
    Scroller.SetSpeed(300);
    break;
  case 620:
    Scroller.SetSpeed(600);
    break;
  case 640:
    Scroller.SetSpeed(900);
    break;
  case 660:
    Scroller.SetSpeed(1200);
    break;
  case 680:
    Scroller.SetSpeed(1512);
    break;
  }

  Scroller.scroll.ExCount = (Scroller.scroll.ExCount + 1) % 880;
}

// ラスタースクロールオープン //
static void ScrollCmdRasterOpen() {
  int i = 0;
  int j = 0;

  // ちょっと重いかもね... //
  for (i = j = 0; i < 31; i++, j += 16) {
    Scroller.scroll.RasterDx[i] = Cast::down<int8_t>(
        sinl((Scroller.scroll.RasterDeg + j), Scroller.scroll.RasterWidth));
  }

  Scroller.scroll.RasterDeg += 2;

  if (Scroller.scroll.RasterWidth < 2) {
    Scroller.scroll.RasterWidth++;
  }
}

// ラスタースクロールクローズ //
static void ScrollCmdRasterClose() {
  int i = 0;
  int j = 0;

  // ちょっと重いかもね... //
  for (i = j = 0; i < 31; i++, j += 2) {
    Scroller.scroll.RasterDx[i] = Cast::down<int8_t>(
        sinl((Scroller.scroll.RasterDeg + j), Scroller.scroll.RasterWidth));
  }

  Scroller.scroll.RasterDeg += 8;

  Scroller.scroll.RasterWidth--;

  if (Scroller.scroll.RasterWidth == 0) {
    Scroller.scroll.ExCmd = ScrollCmdDummy;
  }
}

// ゲイツ雲 //
static void ScrollCmdStg3Boss() {
  Scroller.scroll.ExCount = (Scroller.scroll.ExCount + 200) % 208;
  //	MoveStg3Cloud();
}

// ゲイツ雲描画 //
static void Stg3BossMapDraw() {
  int x = 0;
  int y = 0;

  x = X_MIN;
  y = Y_MIN - Scroller.scroll.ExCount;

  for (; y < Y_MAX; y += 208) {
    constexpr PIXEL_LTRB src = {0, 272, (640 - 256), (272 + 208)};
    GrpSurface_Blit({x, y}, SURFACE_ID::MAPCHIP, src);
  }

  //	DrawStg3Cloud();
}

// ６面の３Ｄキューう゛ //
static void ScrollCmdStg6Cube() { Effects.Move3DCubes(); }

// ６面のランダムＥＣＬ列 //
static void ScrollCmdStg6RndEcl() { Effects.MoveFakeECL(); }

// ６面ラスター //
static void ScrollCmdStg6Raster() { Effects.MoveStg6Rasters(); }

// ４面岩 //
static void ScrollCmdStg4Rock() { Effects.MoveStg4Rocks(); }

// ３面高速星 //
static void ScrollCmdStg3Star() {
  Scroller.scroll.ExCount++;

  if (Scroller.scroll.ExCount == 32) {
    Effects.SetScreenEffect(SCNEFC_WHITEOUT);
  }

  Effects.MoveStg3Stars();
}

// マップデータをロードする(BMP含む) //
bool ScrollManager::Init() {
  ScrollSaveHeader *LayerInfo = nullptr;
  int i = 0;
  static bool bInitialized = false;

  scene.MsgFlag = false;
  scene.ReturnFlag = false;

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
          if(scroll.DataHead != nullptr) {
                  LocalFree(scroll.DataHead);
                  scroll.DataHead = nullptr;
          }
          // そうでなければ、スクロールに関する初期化を行う //
          else if(!InitScrollInfo()) {
                  return false;
          }

          // 解凍を行う(後で、ステージを考慮したものに変更すること) //
          scroll.DataHead = in->MemExpand(0);
          if(!scroll.DataHead) {
                  return false;
          }
  */
  // 標準のスクロールスピード(マップエディタと同じ) //
  auto *head = scroll.DataHead.get();
  scroll.ScrollSpeed = TIME_PER_FRAME; // 標準のスクロール速度
  scroll.NumLayer = U32LEAt(head);     // レイヤーの数
  scroll.Count = 0;                    // スクロール用カウンタ
  scroll.InfStart = 0;                 // 無限ループ開始時刻
  scroll.State = SST_NORMAL;           // 状態(デフォルトの標準で..)
  scroll.IsQuake = 0;                  // 振動中ではない
  scroll.ExCmd = ScrollCmdDummy;       // 特殊コマンド
  scroll.ExCount = 0;
  scroll.RasterWidth = 0;
  scroll.RasterDeg = 0;

  // ラスタースクロールの初期化 //
  for (auto &it : scroll.RasterDx) {
    it = 0;
  }

  // レイヤー情報のロード(長さからループ用変数を調整すること) //
  LayerInfo = reinterpret_cast<ScrollSaveHeader *>(head + sizeof(U32LE));
  for (i = 0; i < scroll.NumLayer; i++) {
    auto *p = reinterpret_cast<PBGMAP *>(head + LayerInfo[i].Address);
    scroll.LayerHead[i] = p;                       // 先頭
    scroll.LayerPtr[i] = p;                        // 現在
    scroll.LayerWait[i] = LayerInfo[i].ScrollWait; // レイヤーの重み
    scroll.LayerCount[i] = 0;
    scroll.LayerDy[i] = 0;
  }

  // 無限ループ終了時刻 //
  scroll.InfEnd = 16 * (LayerInfo[i - 1].Length - (1280 / 16)) *
                  LayerInfo[i - 1].ScrollWait;

  return true;
}

// スクロールに関する情報の初期化を行う //
static void InitMapChipRect() {
  int i = 0;
  int x = 0;
  int y = 0;

  // マップチップ用矩形の準備 //
  for (i = 0; i < 1200; i++) {
    x = (i % (640 / 16)) << 4; // マップエディタと同様の演算式
    y = (i / (640 / 16)) << 4; // マップエディタと同様の演算式
    Scroller.map_chip_rects[i] = {x, y, (x + 16), (y + 16)};
  }
}
