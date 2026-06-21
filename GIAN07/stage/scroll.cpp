///
/// Scroll - Scrolling and map data processing
///

#include "scroll.h"

#include "config.h"
#include "demo_play.h"
#include "audio/bgm.h"
#include "util/cast.h"
#include "util/debug.h"
#include "util/endian.h"
#include "sys/input.h"
#include "audio/snd.h"
#include "util/ut_math.h"
#include "gian.h"
#include "level.h"
#include "gfx/graphics_backend.h"
#include "scene.h" // SCL definition file
#include "ui_manager.h"
#include "window_sys.h"
#include <utility>

// Map data save header
struct ScrollSaveHeader {
  U32LE Address;    // Start address of this data
  U32LE ScrollWait; // Delay for this layer
  U32LE Length;     // Length of this layer
};

// ScrollInfo, SclInfo, Scroller.map_chip_rects[] moved to
// ScrollManager in scroll_manager.cpp

static void enemy_set();                // Set enemies
static void PutEnemy(const uint8_t *p); // p: Enemy placement data after SCL_ENEMY
static void InitMapChipRect();          // Initialize scroll-related information

static PBGMAP *ScNextLine(PBGMAP *p);   // Go to next line!!
static PBGMAP *ScBeforeLine(PBGMAP *p); // Go to previous line!!

static void ScrollCmdDummy();       // No special scroll
static void ScrollCmdStg2Boss();    // Stage 2 boss scroll
static void ScrollCmdRasterOpen();  // Raster scroll open
static void ScrollCmdRasterClose(); // Raster scroll close
static void ScrollCmdStg3Boss();    // Gates cloud

static void ScrollCmdStg6Cube();   // Stage 6 3D cube
static void ScrollCmdStg6RndEcl(); // Stage 6 random ECL array
static void ScrollCmdStg6Raster(); // Stage 6 raster

static void ScrollCmdStg3Star(); // Stage 3 high-speed star

static void Stg3BossMapDraw(); // Gates cloud drawing

static void ScrollCmdStg4Rock(); // Stage 4 rock

// Debug macro
static void SCL_DEBUG(std::string_view s) {
#ifdef SCRIPT_TRACE
  DebugLog(s);
#endif
}

// Move background (one frame)
void ScrollManager::Move() {
  int i = 0;

  enemy_set();    // Set enemies
  scroll.ExCmd(); // Trigger special scroll!!

  // Run quake effect (separate from special scroll)
  if (scroll.IsQuake != 0U) {
    scroll.IsQuake += 2;
  }

  // Advance counter by standard scroll speed
  if (scroll.DataHead == nullptr) {
    return;
  }
  if (scroll.Count >= scroll.InfEnd) {
    return;
  }

  // Return if no scrolling
  if (scroll.ScrollSpeed == 0) {
    return;
  }

  scroll.Count += scroll.ScrollSpeed;

  if (scroll.ScrollSpeed > 0) {
    // Normal scroll
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
    // Reverse scroll
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

// p: Enemy placement data after SCL_ENEMY
// Scroller.key_wait_count moved to Scroller in scroll_manager.cpp

static void enemy_set() {
  bool bFlag = true;
  //	BIT_DEVICE	*in;				// Dangerous...
  bool CtrlFlag = false;

  while (bFlag) {
    const auto *cmd = Enemies.scl_now;
    switch (cmd[0]) {
    case SCL_KEY: // Wait for key input
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
      // If message window is open
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
      //
                                      // if((Key_Data & KEY_SKIP) &&
         // Scroller.scene.MsgFlag)
         // GameState.game_count+=(temp-GameState.game_count)/3; else if((Key_Data
         // & KEY_RETURN) && !Scroller.scene.ReturnFlag){ GameState.game_count  =
         // temp; Scroller.scene.ReturnFlag = true;
                                      // }
                                      // if(!(Key_Data & KEY_RETURN) &&
         // Scroller.scene.ReturnFlag) { Scroller.scene.ReturnFlag = false;
                                      // }
      if (temp > GameState.game_count) {
        bFlag = false;
      } else {
        Enemies.scl_now += 5; // cmd(1)+time(4)
      }
      SCL_DEBUG("--- SCL_TIME ---");
    } break;

    case SCL_ENEMY:
      if (Bosses.count == 0) {
        PutEnemy(cmd + 1); // Don't spawn while boss is active
      }
      Enemies.scl_now += 6; // cmd(1)+x(2)+y(2)+id(1)
      SCL_DEBUG("--- SCL_ENEMY ---");
      break;

    case SCL_BOSS: { // Set boss (X(16),Y(16),ID(8))
      const auto x = I16LEAt(&cmd[1 + 0]); // Boss initial X
      const auto y = I16LEAt(&cmd[1 + 2]); // Boss initial Y
      const auto id = cmd[1 + 2 + 2];      // Boss ID
      Bosses.Set(x, y, id);
      Enemies.scl_now += (1 + 2 + 2 + 1); // cmd+x+y+id

      // Scan SCL ahead for timeout target
      const auto *scl_end =
          Enemies.scl_head.data() + Enemies.scl_head.size();
      const uint8_t *scan = Enemies.scl_now;
      int32_t scl_timeout = -1;
      while (scan < scl_end) {
        const uint8_t op = scan[0];
        if (op == SCL_BOSSDEAD || op == SCL_WAITEX ||
            op == SCL_STAGECLEAR || op == SCL_GAMECLEAR ||
            op == SCL_END) {
          break;
        }
        if (op == SCL_TIME) {
          scl_timeout = static_cast<int32_t>(U32LEAt(&scan[1]));
          scan += 5;
        } else if (op == SCL_ENEMY || op == SCL_BOSS) {
          scan += 6;
        } else if (op == SCL_SSP) {
          scan += 3;
        } else if (op == SCL_EFC) {
          scan += 2;
        } else if (op == SCL_MSG) {
          scan++;
          while (scan < scl_end && *scan != 0)
            scan++;
          if (scan < scl_end)
            scan++;
        } else if (op == SCL_MUSIC || op == SCL_FACE) {
          scan += 2;
        } else if (op == SCL_LOADFACE) {
          scan += 3;
        } else if (op == SCL_STAFF) {
          scan += 2;
        } else {
          scan++; // MWOPEN, MWCLOSE, NPG, KEY, MAPPALETTE,
                  // DELENEMY, ENEMYPALETTE, EXTRACLEAR,
                  // STAGECLEAR/GAMECLEAR/BOSSDEAD/END(handled above),
                  // unknown
        }
      }
      if (scl_timeout > 0) {
        Bosses.SetSCLTimeout(scl_timeout);
      }
    } break;

    case SCL_BOSSDEAD: // Force destroy boss (Level2 command only)
      Bosses.KillAll();
      Enemies.scl_now++;
      break;

    case SCL_MWOPEN: // Open message window
      if (!ConfigDat.msg_disable) {
        UI.Msg().Open();
      }
      Scroller.scene.MsgFlag = true;
      Enemies.scl_now++;
      break;

    case SCL_MWCLOSE: // Close message window
      if (!ConfigDat.msg_disable) {
        UI.Msg().Close();
      }
      Scroller.scene.MsgFlag = false;
      Enemies.scl_now++;
      break;

    case SCL_MSG: // Output message
      // UI.Msg().Cmd(MWCMD_SMALLFONT);
      UI.Msg().Msg(reinterpret_cast<const char *>(cmd + 1));
      Enemies.scl_now += (strlen(reinterpret_cast<const char *>(cmd + 1)) + 2);
      break;

    case SCL_FACE: // Display face
      UI.Msg().Face(cmd[1]);
      Enemies.scl_now += 2;
      break;

    case SCL_LOADFACE: // Load face graphic (SurfaceID, FileNo)
      LoadFace(cmd[1], cmd[2]);
      Enemies.scl_now += 3;
      break;

    case SCL_NPG: // Change to new page
      UI.Msg().Cmd(MWCMD_NEWPAGE);
      Enemies.scl_now++;
      break;

    case SCL_END: // Return without changing count
                  //
                  // UI.Msg().Open();
                  // UI.Msg().Cmd(MWCMD_NEWPAGE);
                  // UI.Msg().Cmd(MWCMD_LARGEFONT);
                  // UI.Msg().Msg("SCL complete");
                  // SCL_DEBUG("--- SCL_END ---");
      return;

    case SCL_SSP: // Change scroll speed
      Scroller.SetSpeed(I16LEAt(&cmd[1]));
      Enemies.scl_now += 3;
      break;

    case SCL_MUSIC:
      //				if(!(// DemoplaySaveEnable||// Demos.load_enable)){
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
        break; // Stop warning
      case SEFC_MUSICFADE:
        BGM_FadeOut(120);
        break; // Stop music
      case SEFC_STG2BOSS:
        Scroller.Command(SCMD_STG2BOSS);
        break; // Stage 2 boss scroll
      case SEFC_RASTERON:
        Scroller.Command(SCMD_RASTER_ON);
        break; // Raster ON
      case SEFC_RASTEROFF:
        Scroller.Command(SCMD_RASTER_OFF);
        break; // Raster OFF
      case SEFC_STG3BOSS:
        Scroller.Command(SCMD_STG3BOSS);
        break; // Stage 3 boss cloud
      case SEFC_STG3RESET:
        Scroller.Command(SCMD_STG3RESET);
        break; // Stage 3 reset
      case SEFC_CFADEIN:
        Effects.SetScreenEffect(SCNEFC_CFADEIN);
        break; // Circle fade in
      case SEFC_CFADEOUT:
        Effects.SetScreenEffect(SCNEFC_CFADEOUT);
        break; // Circle fade out
      case SEFC_STG6CUBE:
        Scroller.Command(SCMD_STG6CUBE);
        break; // Stage 6 cube
      case SEFC_STG6RNDECL:
        Scroller.Command(SCMD_STG6RNDECL);
        break; // Stage 6 ECL array
      case SEFC_STG4ROCK:
        Scroller.Command(SCMD_STG4ROCK);
        break; // Stage 4 rock
      case SEFC_STG4LEAVE:
        Scroller.Command(SCMD_STG4LEAVE);
        break; // Stage 4 rock off-screen
      case SEFC_WHITEIN:
        Effects.SetScreenEffect(SCNEFC_WHITEIN);
        break; // White in
      case SEFC_WHITEOUT:
        Effects.SetScreenEffect(SCNEFC_WHITEOUT);
        break; // White out
      case SEFC_LOADEX01:
        LoadGraph(GRAPH_ID_EXBOSS1);
        break;
      case SEFC_LOADEX02:
        LoadGraph(GRAPH_ID_EXBOSS2);
        break;
      case SEFC_STG6RASTER:
        Scroller.Command(SCMD_STG6RASTER);
        break; // Stage 6 raster
      }
      Enemies.scl_now += 2;
      break;

    case SCL_WAITEX: // Special wait <cmd1>,<opt4>
      switch (cmd[1]) {
      case SWAIT_BOSSHP: // Remaining HP
        if (Bosses.GetHPSum() <= U32LEAt(&cmd[2])) {
          break;
        }
        return;
      case SWAIT_BOSSLEFT: // Remaining boss count
        if (Bosses.count <= U32LEAt(&cmd[2])) {
          break;
        }
        return;
      }
      Enemies.scl_now += (1 + 1 + 4);
      break;

    case SCL_STAGECLEAR: // Stage clear
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
      // Stage clear processing here
      GameNextStage(); // Error checking needed!!
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
      if (GameState.game_level != GameLevel::EASY) {
        switch (Players.Weapon()) {
        case 0:
          ConfigDat.extra_stg_flags |= 1;
          break;
        case 1:
          ConfigDat.extra_stg_flags |= 2;
          break;
        case 2:
          ConfigDat.extra_stg_flags |= 4;
          break;
        }
      }
      ConfigDat.Save();
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

    case SCL_MAPPALETTE: // Palette from map parts Surface
      // Use 40 surrounding colors as map parts palette
      Enemies.scl_now++;
      break;

    case SCL_ENEMYPALETTE:
      LoadPaletteFromMAP(); // Bit depth check delegated to function
      Enemies.scl_now++;
      break;

    default: // Not implemented or bug
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
  // [Note]
  //  p[0-1]:EnemyX  p[2-3]:EnemyY  p[4]:EnemyID
  //  Enemies.ecl_head[0-3]:Num  Enemies.ecl_head[n*4-(n*4+3)]
  // (n>1):StartAddr(ABS)
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

  //
          // e->x   = I16LEAt(&p[0]);	// PixelToWorld(I16LEAt(&p[0]));
          // e->y   = I16LEAt(&p[2]);	// PixelToWorld(I16LEAt(&p[2]));
          //
          // e->x = (e->x==X_RNDV) ? GX_RND() : (e->x<<6);
          // e->y = (e->y==Y_RNDV) ? GY_RND() : (e->y<<6);
          // e->cmd = U32LEAt(&Enemies.ecl_head[n]);
          //
          // e->call_addr = e->cmd;
          //
          // e->hp       = 0xffffffff;
          // e->amp      = 0;
          // e->anm_ptn  = 0;
          // e->anm_sp   = 0;
          // e->anm_c    = 0;
          // e->count    = 0;
          // e->evscore  = 0;
          // e->d        = 64;
          // e->flag     = EF_DAMAGE|EF_DRAW|EF_HITSB;
          //
          // e->tama_c   = rnd();//&0xff;
          // e->t_rep    = 0;			//
      // Bullet fire interval (0: no auto-fire) e->g_width  = 0; e->g_height = 0;
          //
          // e->item     = 0;
          //
          // e->rep_c    = 0;
          // e->cmd_c    = 0;
          // e->v        = 64;
          // e->vd       = 0;
          // e->vx       = cosl(e->d,e->v);
          // e->vy       = sinl(e->d,e->v);
          //
          // e->LLaserRef = 0;
          //
          // e->t_cmd.c      = 0;
          // e->t_cmd.cmd    = TC_WAY;
          // e->t_cmd.d      = 64;
          // e->t_cmd.n      = 1;
          // e->t_cmd.option = TE_NONE;
          // e->t_cmd.type   = T_NORM;
          // e->t_cmd.v      = 3;
          // e->t_cmd.x      = 0;
          // e->t_cmd.y      = 0;
          //
          // e->t_cmd.dw     = 16;
          // e->t_cmd.ns     = 1;
          // e->t_cmd.rep    = 0;
          // e->t_cmd.vd     = 0;
          //
          //
          // // Initialize variable registers //
          // e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
          // e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;
          //
          // // Initialize interrupt vectors //
          // Enemies.InitInterrupts(e);
}

//
// static void ExDraw(void)
// {
//         int			x,y;
//         int			dx,dy;
//         int			infx,infy;
//         int			i,j;
//
//         int			ox,oy;
//
//         static BYTE		deg = 0;
//         static int		len = 0;
//         static BYTE		count = 0;
//         static char		flag = 1;
//
//         ox = cosl(deg,len*flag)+320;
//         oy = sinl(deg,len*flag)+240;
//
//         count++;
//
//         if(count==0)	flag *= -1;
//
//         if(count>60)	len = (len+4)%64;
//         else			deg+=4;
//
//         GrpGeom->Lock();
//         GrpGeom->SetColor({ 0, 3, 0 });
//
//         dx = cosl(deg+64,64);		infx = cosl(deg,800);
//         dy = sinl(deg+64,64);		infy = sinl(deg,800);
//         for(i=-10;i<=10;i++){
//                 x = ox + dx*i;
//                 y = oy + dy*i;
//                 GrpGeom->DrawLine((x - infx), (y - infy), (x + infx), (y +
// infy));
//         }
//
//         dx = cosl(deg,64);		infx = cosl(deg+64,800);
//         dy = sinl(deg,64);		infy = sinl(deg+64,800);
//         for(i=-10;i<=10;i++){
//                 x = ox + dx*i;
//                 y = oy + dy*i;
//                 GrpGeom->DrawLine((x - infx), (y - infy), (x + infx), (y +
// infy));
//         }
//
//         GrpGeom->Unlock();
// }

// Draw background
void ScrollManager::Draw() {
  PBGMAP *p = nullptr;
  int i = 0;
  int j = 0;
  int k = 0;
  int x = 0;
  int y = 0;
  int dx = 0;       // For quake
  int RasterDx = 0; // For raster

  if (scroll.DataHead == nullptr) {
    return;
  }

  // Exception: Wide shot bomb active
  // if(Players.IsBombActive() && Players.Weapon()==0){
  //	return;
  //}

  // Special draw mode
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

  // Quake effect
  // dx = sinl(scroll.IsQuake*4,2);
  // if(scroll.IsQuake) dx =
  // sinl(scroll.IsQuake*8+i*6,(256-scroll.IsQuake)>>2);	//4
  if (scroll.IsQuake != 0U) {
    dx = sinl(scroll.IsQuake * 16, (256 - scroll.IsQuake) >> 5); // 4
  }

  // Display all layers
  for (k = 0; k < scroll.NumLayer; k++) {
    p = scroll.LayerPtr[k];
    for (i = 29; i >= -1; i--) {
      RasterDx = (k == 0) ? scroll.RasterDx[i + 1] : 0;
      for (j = 0; j < MAP_WIDTH;) {
        // Normal drawing
        if (*p != MAPDATA_NONE) {
          x = (j << 4) + X_MIN + dx + RasterDx;
          y = (i << 4) + scroll.LayerDy[k];
          const auto &src = map_chip_rects[*p];
          GrpSurface_Blit({x, y}, SURFACE_ID::MAPCHIP, src);
          p++, j++;
        }
        // Empty case
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
  //
          // if(scroll.ExCmd==ScrollCmdStg2Boss){
          //         ExDraw();
          // }
}

// Change scroll speed (arg: (1) scroll speed)
void ScrollManager::SetSpeed(int speed) {
  // if(speed<0) speed = 0;		// Reverse scroll prohibited!!

  scroll.ScrollSpeed = speed;
}

// SCL command execution function (arg: (1) scroll command)
void ScrollManager::Command(uint8_t cmd) {
  switch (cmd) {
  case SCMD_QUAKE: // Quake effect
    scroll.IsQuake = 2;
    break;

  case SCMD_STG2BOSS: // Stage 2 boss
    scroll.ExCmd = ScrollCmdStg2Boss;
    scroll.ExCount = 0;
    break;

  case SCMD_STG3BOSS: // Stage 3 boss
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

  case SCMD_RASTER_ON: // Start raster scroll
    scroll.ExCmd = ScrollCmdRasterOpen;
    scroll.RasterDeg = 0;
    scroll.RasterWidth = 0;
    break;

  case SCMD_RASTER_OFF: // End raster scroll
    scroll.ExCmd = ScrollCmdRasterClose;
    // scroll.RasterDeg   = 0;
    // scroll.RasterWidth = 0;
    break;
  }
}

// No special scroll
static void ScrollCmdDummy() {
  // Does nothing...
}

// Stage 2 boss scroll
static void ScrollCmdStg2Boss() {
  //
  //         SSP	-810	TR	10			SSP	-630	TR
      // 10 SSP	-450	TR	10			SSP	-270	TR
      // 10 SSP	-180	TR	10			SSP	-90
      // TR	10 SSP	0		TR	10			SSP	90
      // TR	10 SSP	180		TR	10			SSP	450
      // TR	10 SSP	630		TR	10			SSP	810
      // TR	300
  //
  // Branch by special timer
  switch (Scroller.scroll.ExCount) {
  // Forward -> reverse
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

  // Reverse -> forward
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

// Raster scroll open
static void ScrollCmdRasterOpen() {
  int i = 0;
  int j = 0;

  // Might be a bit heavy...
  for (i = j = 0; i < 31; i++, j += 16) {
    Scroller.scroll.RasterDx[i] = Cast::down<int8_t>(
        sinl((Scroller.scroll.RasterDeg + j), Scroller.scroll.RasterWidth));
  }

  Scroller.scroll.RasterDeg += 2;

  if (Scroller.scroll.RasterWidth < 2) {
    Scroller.scroll.RasterWidth++;
  }
}

// Raster scroll close
static void ScrollCmdRasterClose() {
  int i = 0;
  int j = 0;

  // Might be a bit heavy...
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

// Gates cloud
static void ScrollCmdStg3Boss() {
  Scroller.scroll.ExCount = (Scroller.scroll.ExCount + 200) % 208;
  //	MoveStg3Cloud();
}

// Gates cloud drawing
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

// Stage 6 3D cube
static void ScrollCmdStg6Cube() { Effects.Move3DCubes(); }

// Stage 6 random ECL array
static void ScrollCmdStg6RndEcl() { Effects.MoveFakeECL(); }

// Stage 6 raster
static void ScrollCmdStg6Raster() { Effects.MoveStg6Rasters(); }

// Stage 4 rock
static void ScrollCmdStg4Rock() { Effects.MoveStg4Rocks(); }

// Stage 3 high-speed star
static void ScrollCmdStg3Star() {
  Scroller.scroll.ExCount++;

  if (Scroller.scroll.ExCount == 32) {
    Effects.SetScreenEffect(SCNEFC_WHITEOUT);
  }

  Effects.MoveStg3Stars();
}

// Load map data (including BMP)
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

  //
          // Prepare loading
          // auto in = FilStartR("GIAN_MAP.DAT");
          // if(!in) {
          //         return false;
          // }
          //
          // Free memory if already loaded
          // if(scroll.DataHead != nullptr) {
          //         LocalFree(scroll.DataHead);
          //         scroll.DataHead = nullptr;
          // }
          // Otherwise, initialize scroll info
          // else if(!InitScrollInfo()) {
          //         return false;
          // }
          //
          // Decompress (TODO: change to stage-aware version)
          // scroll.DataHead = in->MemExpand(0);
          // if(!scroll.DataHead) {
          //         return false;
          // }
  // Standard scroll speed (same as map editor)
  auto *head = scroll.DataHead.get();
  scroll.ScrollSpeed = TIME_PER_FRAME; // Standard scroll speed
  scroll.NumLayer = U32LEAt(head);     // Number of layers
  scroll.Count = 0;                    // Scroll counter
  scroll.InfStart = 0;                 // Infinite loop start time
  scroll.State = SST_NORMAL;           // State (default standard)
  scroll.IsQuake = 0;                  // Not quaking
  scroll.ExCmd = ScrollCmdDummy;       // Special command
  scroll.ExCount = 0;
  scroll.RasterWidth = 0;
  scroll.RasterDeg = 0;

  // Initialize raster scroll
  for (auto &it : scroll.RasterDx) {
    it = 0;
  }

  // Load layer info (adjust loop variables from length)
  LayerInfo = reinterpret_cast<ScrollSaveHeader *>(head + sizeof(U32LE));
  for (i = 0; i < scroll.NumLayer; i++) {
    auto *p = reinterpret_cast<PBGMAP *>(head + LayerInfo[i].Address);
    scroll.LayerHead[i] = p;                       // Head
    scroll.LayerPtr[i] = p;                        // Current
    scroll.LayerWait[i] = LayerInfo[i].ScrollWait; // Layer weight
    scroll.LayerCount[i] = 0;
    scroll.LayerDy[i] = 0;
  }

  // Infinite loop end time
  scroll.InfEnd = 16 * (LayerInfo[i - 1].Length - (1280 / 16)) *
                  LayerInfo[i - 1].ScrollWait;

  return true;
}

// Initialize scroll-related information
static void InitMapChipRect() {
  int i = 0;
  int x = 0;
  int y = 0;

  // Prepare map chip rectangles
  for (i = 0; i < 1200; i++) {
    x = (i % (640 / 16)) << 4; // Same formula as map editor
    y = (i / (640 / 16)) << 4; // Same formula as map editor
    Scroller.map_chip_rects[i] = {x, y, (x + 16), (y + 16)};
  }
}
