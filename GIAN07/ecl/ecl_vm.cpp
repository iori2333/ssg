/*
 *   EclVM — Enemy Control Language bytecode interpreter
 *
 *   Extracted from ENEMY.cpp. Executes ECL bytecode for individual enemies,
 *   handling movement, bullet/laser firing, flag toggles, register arithmetic,
 *   control flow, and interrupt vectors.
 */

#include "ecl/ecl_vm.h"
#include "ecl/ecl_commands.h"
#include "effect/EFFECT.h"
#include "effect/EFFECT3D.h"
#include "enemy/BOSS.h"
#include "enemy/ENEMY.h"
#include "entity/HOMINGL.h"
#include "entity/LASER.h"
#include "entity/LLASER.h"
#include "entity/MAID.h"
#include "entity/TAMA.h"
#include "game/GIAN.h"
#include "game/LEVEL.h"
#include "game/PRankCtrl.h"
#include "game/SCROLL.h"
#include "game/cast.h"
#include "game/coords.h"
#include "game/debug.h"
#include "game/endian.h"
#include "game/snd.h"
#include "game/ut_math.h"

// Special angles (defined in enemy_manager.cpp)
extern uint8_t EnemyEXDEG;
extern uint8_t EnemyEXDEG_D;

// Global instance
std::optional<EclVM> EclVM::s_instance;

void EclVM::Init(std::span<const uint8_t> ecl_data) {
  if (!ecl_data.empty()) {
    s_instance.emplace(ecl_data);
  } else {
    s_instance.reset();
  }
}

void EclVM::Clear() { s_instance.reset(); }

EclVM &EclVM::Instance() { return *s_instance; }

bool EclVM::IsInitialized() { return s_instance.has_value(); }

// Resolve an EclReg register/field ID to its runtime value
uint32_t EclVM::ResolveValue(const EnemyData *e, EclReg id) {
  switch (id) {
  // レジスタ指定の場合 //
  case (EclReg::GR0):
  case (EclReg::GR1):
  case (EclReg::GR2):
  case (EclReg::GR3):
  case (EclReg::GR4):
  case (EclReg::GR5):
  case (EclReg::GR6):
  case (EclReg::GR7):
    return e->GR[std::to_underlying(id)];

  case (EclReg::LCMD_D):
    return e->l_cmd.d; // レーザーコマンド(角度)
  case (EclReg::LCMD_DW):
    return e->l_cmd.dw; // レーザーコマンド(角度差)
  case (EclReg::LCMD_N):
    return e->l_cmd.n; // レーザーコマンド(本数)
  case (EclReg::LCMD_C):
    return e->l_cmd.c; // レーザーコマンド(色)
  case (EclReg::LCMD_L):
    return e->l_cmd.l; // レーザーコマンド(長さ)
  case (EclReg::LCMD_V):
    return e->l_cmd.v; // レーザーコマンド(速度)

  case (EclReg::TCMD_D):
    return e->t_cmd.d; // 弾コマンド(角度)
  case (EclReg::TCMD_DW):
    return e->t_cmd.dw; // 弾コマンド(角度差)
  case (EclReg::TCMD_N):
    return e->t_cmd.n; // 弾コマンド(個数)
  case (EclReg::TCMD_NS):
    return e->t_cmd.ns; // 弾コマンド(連射数)
  case (EclReg::TCMD_V):
    return e->t_cmd.v; // 弾コマンド(速度)
  case (EclReg::TCMD_C):
    return e->t_cmd.c; // 弾コマンド(色)
  case (EclReg::TCMD_A):
    return e->t_cmd.a; // 弾コマンド(加速度)
  case (EclReg::TCMD_REP):
    return e->t_cmd.rep; // 弾コマンド(繰り返し)
  case (EclReg::TCMD_VD):
    return e->t_cmd.vd; // 弾コマンド(角速度)

  case (EclReg::ENEMY_X):
    return e->x; // 敵のＸ座標
  case (EclReg::ENEMY_Y):
    return e->y; // 敵のＹ座標
  case (EclReg::ENEMY_D):
    return e->d; // 敵の角度

  default:
    return 0;
  }
}

void EclVM::Execute(EnemyData &e) {
  // Direction-flip helpers (formerly macros)
  auto AbsDegRL = [&e](uint8_t d) -> uint8_t {
    return (e.flag & EF_RLCHG) ? uint8_t(128 - d) : d;
  };
  auto AbsVxRL = [&e](int vx) { return (e.flag & EF_RLCHG) ? (-vx) : vx; };
  auto RelDegRL = [&e](int8_t d) -> int8_t {
    return (e.flag & EF_RLCHG) ? int8_t(-d) : d;
  };

  int RegCmp = 0;
  HLaserInfo HInfo;

  const PIXEL_LTRB rcDegX2 = {
      GX_MIN + 150 * 64, GY_MIN + (GY_MID - GY_MIN - 40 * 64) / 3,
      GX_MAX - 150 * 64, GY_MID - (GY_MID - GY_MIN - 40 * 64) / 3 - 40 * 64};
  uint16_t BaseAngle, DeltaAngle;

ECL_HEAD:
  const auto *raw = m_ecl_data.data() + e.cmd;
  const EclOp op = static_cast<EclOp>(*raw);

  switch (op) {
  // ============================================================
  // 0x0?: Control flow
  // ============================================================
  case EclOp::SETUP: {
    auto c = Decode(EclOpTag<EclOp::SETUP>{}, raw);
    e.hp = c.hp;
    e.score = c.score;
    if (e.hp == 0)
      BossKillAll();
    break;
  }
  case EclOp::END:
    if (e.LLaserRef)
      LLaserForceClose(&e);
    e.flag = EF_DELETE;
    return;

  case EclOp::JMP: {
    auto c = Decode(EclOpTag<EclOp::JMP>{}, raw);
    e.cmd = c.target;
    goto ECL_HEAD;
  }
  case EclOp::LOOP: {
    auto c = Decode(EclOpTag<EclOp::LOOP>{}, raw);
    if (e.rep_c == 0)
      e.rep_c = c.count + 1;
    if (--e.rep_c != 0) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::CALL: {
    auto c = Decode(EclOpTag<EclOp::CALL>{}, raw);
    e.call_addr = e.cmd + CmdLength(EclOp::CALL);
    e.cmd = c.target;
    goto ECL_HEAD;
  }
  case EclOp::RET:
    e.cmd = e.call_addr;
    goto ECL_HEAD;

  case EclOp::JHPL: {
    auto c = Decode(EclOpTag<EclOp::JHPL>{}, raw);
    if (e.hp > c.value) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::JHPS: {
    auto c = Decode(EclOpTag<EclOp::JHPS>{}, raw);
    if (e.hp < c.value) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::JDIF: {
    auto c = Decode(EclOpTag<EclOp::JDIF>{}, raw);
    switch (PlayRank.GameLevel) {
    case GAME_EASY:
      e.cmd = c.t_easy;
      break;
    default:
    case GAME_NORMAL:
      e.cmd = c.t_norm;
      break;
    case GAME_HARD:
      e.cmd = c.t_hard;
      break;
    case GAME_LUNATIC:
      e.cmd = c.t_luna;
      break;
    }
    goto ECL_HEAD;
  }
  case EclOp::JDSB: {
    auto c = Decode(EclOpTag<EclOp::JDSB>{}, raw);
    uint8_t temp = abs(atan8(Viv.x - e.x, Viv.y - e.y) - e.d);
    if (temp < 4) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::JFCL: {
    auto c = Decode(EclOpTag<EclOp::JFCL>{}, raw);
    if (e.count > c.value) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::JFCS: {
    auto c = Decode(EclOpTag<EclOp::JFCS>{}, raw);
    if (e.count < c.value) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::STI: {
    auto c = Decode(EclOpTag<EclOp::STI>{}, raw);
    switch (static_cast<EclIntVec>(c.cond)) {
    case EclIntVec::BITLEFT:
      e.Vect[std::to_underlying(EclIntVec::BITLEFT)].vect = c.addr;
      e.Vect[std::to_underlying(EclIntVec::BITLEFT)].value = c.value;
      break;
    case EclIntVec::BOSSLEFT:
      e.Vect[std::to_underlying(EclIntVec::BOSSLEFT)].vect = c.addr;
      e.Vect[std::to_underlying(EclIntVec::BOSSLEFT)].value = c.value;
      break;
    case EclIntVec::HP:
      e.Vect[std::to_underlying(EclIntVec::HP)].vect = c.addr;
      e.Vect[std::to_underlying(EclIntVec::HP)].value = c.value;
      break;
    case EclIntVec::TIMER:
      e.Vect[std::to_underlying(EclIntVec::TIMER)].vect = c.addr;
      e.Vect[std::to_underlying(EclIntVec::TIMER)].value = c.value;
      e.IntTimer = 0;
      break;
    default:
      break;
    }
    break;
  }
  case EclOp::CLI: {
    auto c = Decode(EclOpTag<EclOp::CLI>{}, raw);
    switch (static_cast<EclIntVec>(c.vec)) {
    case EclIntVec::BITLEFT:
      e.Vect[std::to_underlying(EclIntVec::BITLEFT)].vect = 0;
      break;
    case EclIntVec::BOSSLEFT:
      e.Vect[std::to_underlying(EclIntVec::BOSSLEFT)].vect = 0;
      break;
    case EclIntVec::HP:
      e.Vect[std::to_underlying(EclIntVec::HP)].vect = 0;
      break;
    case EclIntVec::TIMER:
      e.Vect[std::to_underlying(EclIntVec::TIMER)].vect = 0;
      break;
    default:
      break;
    }
    break;
  }

  // ============================================================
  // 0x1?: Movement
  // ============================================================
  case EclOp::NOP: {
    auto c = Decode(EclOpTag<EclOp::NOP>{}, raw);
    if (e.cmd_c == 0)
      e.cmd_c = c.frames + 1;
    if (--e.cmd_c != 0)
      return;
    break;
  }
  case EclOp::NOPSC: {
    auto c = Decode(EclOpTag<EclOp::NOPSC>{}, raw);
    if (e.cmd_c == 0)
      e.cmd_c = c.frames + 1;
    if (--e.cmd_c != 0)
      return;
    break;
  }
  case EclOp::MOV: {
    auto c = Decode(EclOpTag<EclOp::MOV>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = cosl(e.d, e.v);
      e.vy = sinl(e.d, e.v);
    }
    if (--e.cmd_c != 0) {
      e.x += e.vx;
      e.y += e.vy;
      return;
    }
    break;
  }
  case EclOp::ROL: {
    auto c = Decode(EclOpTag<EclOp::ROL>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vd = RelDegRL(c.deg_delta);
    }
    if (--e.cmd_c != 0) {
      e.x += cosl(e.d, e.v);
      e.y += sinl(e.d, e.v);
      e.d += e.vd;
      return;
    }
    break;
  }
  case EclOp::LROL: {
    auto c = Decode(EclOpTag<EclOp::LROL>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = AbsVxRL(c.vx);
      e.vy = c.vy;
      e.vd = RelDegRL(c.deg_delta);
    }
    if (--e.cmd_c != 0) {
      e.x += cosl(e.d, e.v) + e.vx;
      e.y += sinl(e.d, e.v) + e.vy;
      e.d += e.vd;
      return;
    }
    break;
  }
  case EclOp::WAVX: {
    auto c = Decode(EclOpTag<EclOp::WAVX>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = AbsVxRL(c.vx);
      e.vy = e.y;
      e.amp = c.amp;
      e.vd = c.deg_delta;
    }
    if (--e.cmd_c != 0) {
      e.x += e.vx;
      e.y = e.vy + sinl(e.d, e.amp << 6);
      e.d += e.vd;
      return;
    }
    break;
  }
  case EclOp::WAVY: {
    auto c = Decode(EclOpTag<EclOp::WAVY>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vy = c.vy;
      e.vx = e.x;
      e.amp = c.amp;
      e.vd = c.deg_delta;
    }
    if (--e.cmd_c != 0) {
      e.y += e.vy;
      e.x = e.vx + sinl(e.d, e.amp << 6);
      e.d += e.vd;
      return;
    }
    break;
  }
  case EclOp::MXA: {
    auto c = Decode(EclOpTag<EclOp::MXA>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = (PixelToWorld(c.target_x) - e.x) / e.cmd_c;
      e.vy = 0;
    }
    if (--e.cmd_c != 0) {
      e.x += e.vx;
      return;
    }
    break;
  }
  case EclOp::MYA: {
    auto c = Decode(EclOpTag<EclOp::MYA>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vy = (PixelToWorld(c.target_y) - e.y) / e.cmd_c;
      e.vx = 0;
    }
    if (--e.cmd_c != 0) {
      e.y += e.vy;
      return;
    }
    break;
  }
  case EclOp::MXYA: {
    auto c = Decode(EclOpTag<EclOp::MXYA>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = (PixelToWorld(c.target_x) - e.x) / e.cmd_c;
      e.vy = (PixelToWorld(c.target_y) - e.y) / e.cmd_c;
    }
    if (--e.cmd_c != 0) {
      e.x += e.vx;
      e.y += e.vy;
      return;
    }
    break;
  }
  case EclOp::MXS: {
    auto c = Decode(EclOpTag<EclOp::MXS>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = (Viv.x - e.x) / e.cmd_c;
      e.vy = 0;
    }
    if (--e.cmd_c != 0) {
      e.x += e.vx;
      return;
    }
    break;
  }
  case EclOp::MYS: {
    auto c = Decode(EclOpTag<EclOp::MYS>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = 0;
      e.vy = (Viv.y - e.y) / e.cmd_c;
    }
    if (--e.cmd_c != 0) {
      e.y += e.vy;
      return;
    }
    break;
  }
  case EclOp::MXYS: {
    auto c = Decode(EclOpTag<EclOp::MXYS>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = c.frames + 1;
      e.vx = (Viv.x - e.x) / e.cmd_c;
      e.vy = (Viv.y - e.y) / e.cmd_c;
    }
    if (--e.cmd_c != 0) {
      e.x += e.vx;
      e.y += e.vy;
      return;
    }
    break;
  }
  case EclOp::ACC: {
    auto c = Decode(EclOpTag<EclOp::ACC>{}, raw);
    if (e.cmd_c == 0)
      e.cmd_c = c.frames + 1;
    if (--e.cmd_c != 0) {
      e.v += c.accel;
      e.x += cosl(e.d, e.v);
      e.y += sinl(e.d, e.v);
      return;
    }
    break;
  }
  case EclOp::ACCXYA:
    e.cmd += CmdLength(EclOp::ACCXYA);
    return; // unimplemented, stop for frame

  case EclOp::GRAX: {
    auto c = Decode(EclOpTag<EclOp::GRAX>{}, raw);
    if (e.cmd_c == 0) {
      e.cmd_c = 9999;
      e.vx = cosl(e.d, e.v);
      e.vy = sinl(e.d, e.v);
      e.vd = c.gravity;
      e.flag |= EF_CLIP;
    } else {
      e.x += e.vx;
      e.y += e.vy;
      e.vy += e.vd;
      if (e.x < GX_MIN || e.x > GX_MAX) {
        e.vx = -e.vx;
        e.x += e.vx;
      }
      if (e.y < GY_MIN) {
        e.vy = -e.vy;
        e.y += e.vy;
      }
      if (e.y > GY_MAX + e.g_height) {
        e.flag = EF_DELETE;
      }
    }
    return;
  }

  // ============================================================
  // 0x2?: Value assignment
  // ============================================================
  case EclOp::DEGA: {
    auto c = Decode(EclOpTag<EclOp::DEGA>{}, raw);
    e.d = AbsDegRL(c.deg);
    break;
  }
  case EclOp::DEGR: {
    auto c = Decode(EclOpTag<EclOp::DEGR>{}, raw);
    e.d += RelDegRL(c.delta);
    break;
  }
  case EclOp::DEGX:
    e.d = rnd() & 0xff;
    break;
  case EclOp::DEGXU:
    e.d = 128 + (rnd() & 0x7f);
    break;
  case EclOp::DEGXD:
    e.d = rnd() & 0x7f;
    break;
  case EclOp::DEGEX:
    e.d = EnemyEXDEG;
    EnemyEXDEG += EnemyEXDEG_D;
    break;
  case EclOp::DEGS:
    e.d = atan8(Viv.x - e.x, Viv.y - e.y);
    break;

  case EclOp::SPDA: {
    auto c = Decode(EclOpTag<EclOp::SPDA>{}, raw);
    e.v = c.v;
    break;
  }
  case EclOp::SPDR: {
    auto c = Decode(EclOpTag<EclOp::SPDR>{}, raw);
    e.v += c.delta;
    break;
  }
  case EclOp::XYA: {
    auto c = Decode(EclOpTag<EclOp::XYA>{}, raw);
    e.x = PixelToWorld(c.x);
    e.y = PixelToWorld(c.y);
    break;
  }
  case EclOp::XYR: {
    auto c = Decode(EclOpTag<EclOp::XYR>{}, raw);
    e.x += PixelToWorld(c.dx);
    e.y += PixelToWorld(c.dy);
    break;
  }
  case EclOp::XYS:
    e.x = Viv.x;
    e.y = Viv.y;
    break;

  case EclOp::DEGX2: {
    if (e.y < rcDegX2.top) {
      if (e.x < rcDegX2.left) {
        BaseAngle = 16;
        DeltaAngle = 32;
      } else if (e.x > rcDegX2.right) {
        BaseAngle = 80;
        DeltaAngle = 32;
      } else {
        BaseAngle = 32 + ((rnd() >> 1) & 1) * 64 - 16;
        DeltaAngle = 32;
      }
    } else if (e.y > rcDegX2.bottom) {
      if (e.x < rcDegX2.left) {
        BaseAngle = -48;
        DeltaAngle = 32;
      } else if (e.x > rcDegX2.right) {
        BaseAngle = 144;
        DeltaAngle = 32;
      } else {
        BaseAngle = 176;
        DeltaAngle = 32;
      }
    } else {
      if (e.x < rcDegX2.left) {
        BaseAngle = -16;
        DeltaAngle = 32;
      } else if (e.x > rcDegX2.right) {
        BaseAngle = 112;
        DeltaAngle = 32;
      } else {
        BaseAngle = ((rnd() >> 1) & 1) ? -16 : 112;
        DeltaAngle = 32;
      }
    }
    e.d = BaseAngle + (rnd() >> 1) % DeltaAngle;
    break;
  }
  case EclOp::XYRND:
    if (e.x > GX_MID)
      e.x = X_MID * 64 - (rnd() % (X_MAX - X_MIN - 100)) * 32;
    else
      e.x = X_MID * 64 + (rnd() % (X_MAX - X_MIN - 100)) * 32;
    e.y = (rnd() % (Y_MID - Y_MIN - 160)) * 64 + (Y_MIN + 40) * 64;
    break;

  case EclOp::XYL: {
    auto c = Decode(EclOpTag<EclOp::XYL>{}, raw);
    e.x += cosl(e.d, PixelToWorld(c.length));
    e.y += sinl(e.d, PixelToWorld(c.length));
    break;
  }

  // ============================================================
  // 0x4?: Bullet firing
  // ============================================================
  case EclOp::TAMA:
    TamaCmd = e.t_cmd;
    TamaCmd.x += e.x;
    TamaCmd.y += e.y;
    tama_set();
    break;
  case EclOp::TAMA2:
    TamaCmd = e.t_cmd;
    TamaCmd.x += e.x;
    TamaCmd.y += e.y;
    tama_setEX();
    break;
  case EclOp::TAMAL:
    TamaCmd = e.t_cmd;
    TamaCmd.x += e.x;
    TamaCmd.y += e.y;
    tama_setLine();
    break;
  case EclOp::TAMAEX:
    TamaCmd = e.t_cmd;
    TamaCmd.x += e.x;
    TamaCmd.y += e.y;
    tama_setExtra01();
    break;
  case EclOp::TCLR:
    BossClearCmd();
    tama_clear();
    laser_clear();
    HLaserClear();
    enemy_clear();
    break;

  case EclOp::TAUTO: {
    auto c = Decode(EclOpTag<EclOp::TAUTO>{}, raw);
    e.t_rep = c.interval;
    break;
  }
  case EclOp::TXYR: {
    auto c = Decode(EclOpTag<EclOp::TXYR>{}, raw);
    e.t_cmd.x = PixelToWorld(c.x);
    e.t_cmd.y = PixelToWorld(c.y);
    break;
  }
  case EclOp::TCMD: {
    auto c = Decode(EclOpTag<EclOp::TCMD>{}, raw);
    e.t_cmd.cmd = c.cmd;
    break;
  }
  case EclOp::TDEGA: {
    auto c = Decode(EclOpTag<EclOp::TDEGA>{}, raw);
    e.t_cmd.d = c.d;
    e.t_cmd.dw = c.dw;
    break;
  }
  case EclOp::TDEGR: {
    auto c = Decode(EclOpTag<EclOp::TDEGR>{}, raw);
    e.t_cmd.d += c.dd;
    e.t_cmd.dw += c.ddw;
    break;
  }
  case EclOp::TDEGS:
    e.t_cmd.d = atan8(Viv.x - e.x, Viv.y - e.y);
    break;
  case EclOp::TDEGE:
    e.t_cmd.d = e.d;
    break;
  case EclOp::TNUMA: {
    auto c = Decode(EclOpTag<EclOp::TNUMA>{}, raw);
    e.t_cmd.n = c.n;
    e.t_cmd.ns = c.ns;
    break;
  }
  case EclOp::TNUMR: {
    auto c = Decode(EclOpTag<EclOp::TNUMR>{}, raw);
    e.t_cmd.n += c.dn;
    e.t_cmd.ns += c.dns;
    break;
  }
  case EclOp::TSPDA: {
    auto c = Decode(EclOpTag<EclOp::TSPDA>{}, raw);
    e.t_cmd.v = c.v;
    e.t_cmd.a = c.a;
    break;
  }
  case EclOp::TSPDR: {
    auto c = Decode(EclOpTag<EclOp::TSPDR>{}, raw);
    auto temp = e.t_cmd.v;
    e.t_cmd.v = ((temp & 0x3f) + c.dv) & 0x3f;
    e.t_cmd.v |= (temp & 0xc0);
    e.t_cmd.a += c.da;
    break;
  }
  case EclOp::TOPT: {
    auto c = Decode(EclOpTag<EclOp::TOPT>{}, raw);
    e.t_cmd.option = c.option;
    break;
  }
  case EclOp::TTYPE: {
    auto c = Decode(EclOpTag<EclOp::TTYPE>{}, raw);
    e.t_cmd.type = c.type;
    break;
  }
  case EclOp::TCOL: {
    auto c = Decode(EclOpTag<EclOp::TCOL>{}, raw);
    e.t_cmd.c = c.color;
    break;
  }
  case EclOp::TVDEG: {
    auto c = Decode(EclOpTag<EclOp::TVDEG>{}, raw);
    e.t_cmd.vd = c.vd;
    break;
  }
  case EclOp::TREP: {
    auto c = Decode(EclOpTag<EclOp::TREP>{}, raw);
    e.t_cmd.rep = c.rep;
    break;
  }
  case EclOp::T2ITEM: {
    auto c = Decode(EclOpTag<EclOp::T2ITEM>{}, raw);
    tama2item(c.pct);
    break;
  }

  // ============================================================
  // 0x6?: Laser firing
  // ============================================================
  case EclOp::LASER:
    LaserCmd = e.l_cmd;
    LaserCmd.x += e.x;
    LaserCmd.y += e.y;
    laser_set();
    break;
  case EclOp::LASER2:
    LaserCmd = e.l_cmd;
    LaserCmd.x += e.x;
    LaserCmd.y += e.y;
    laser_setEX();
    break;

  case EclOp::LCMD: {
    auto c = Decode(EclOpTag<EclOp::LCMD>{}, raw);
    e.l_cmd.cmd = c.cmd;
    break;
  }
  case EclOp::LLA: {
    auto c = Decode(EclOpTag<EclOp::LLA>{}, raw);
    e.l_cmd.l = c.length;
    break;
  }
  case EclOp::LLR: {
    auto c = Decode(EclOpTag<EclOp::LLR>{}, raw);
    e.l_cmd.l += c.delta;
    break;
  }
  case EclOp::LL2: {
    auto c = Decode(EclOpTag<EclOp::LL2>{}, raw);
    e.l_cmd.l2 = c.offset;
    break;
  }
  case EclOp::LDEGA: {
    auto c = Decode(EclOpTag<EclOp::LDEGA>{}, raw);
    e.l_cmd.d = c.d;
    e.l_cmd.dw = c.dw;
    break;
  }
  case EclOp::LDEGR: {
    auto c = Decode(EclOpTag<EclOp::LDEGR>{}, raw);
    e.l_cmd.d += c.dd;
    e.l_cmd.dw += c.ddw;
    break;
  }
  case EclOp::LDEGS:
    e.l_cmd.d = atan8(Viv.x - e.x, Viv.y - e.y);
    break;
  case EclOp::LDEGE:
    e.l_cmd.d = e.d;
    break;
  case EclOp::LNUMA: {
    auto c = Decode(EclOpTag<EclOp::LNUMA>{}, raw);
    e.l_cmd.n = c.n;
    break;
  }
  case EclOp::LNUMR: {
    auto c = Decode(EclOpTag<EclOp::LNUMR>{}, raw);
    e.l_cmd.n += c.dn;
    break;
  }
  case EclOp::LSPDA: {
    auto c = Decode(EclOpTag<EclOp::LSPDA>{}, raw);
    e.l_cmd.v = c.v;
    break;
  }
  case EclOp::LSPDR: {
    auto c = Decode(EclOpTag<EclOp::LSPDR>{}, raw);
    e.l_cmd.v = c.dv;
    break;
  }
  case EclOp::LCOL: {
    auto c = Decode(EclOpTag<EclOp::LCOL>{}, raw);
    e.l_cmd.c = c.color;
    break;
  }
  case EclOp::LTYPE: {
    auto c = Decode(EclOpTag<EclOp::LTYPE>{}, raw);
    e.l_cmd.type = c.type;
    break;
  }
  case EclOp::LWA: {
    auto c = Decode(EclOpTag<EclOp::LWA>{}, raw);
    e.l_cmd.w = c.width;
    break;
  }
  case EclOp::LXY: {
    auto c = Decode(EclOpTag<EclOp::LXY>{}, raw);
    e.l_cmd.x = PixelToWorld(c.x);
    e.l_cmd.y = PixelToWorld(c.y);
    break;
  }

  // ============================================================
  // 0x8?: Large laser & homing
  // ============================================================
  case EclOp::LLSET: {
    LLaserCmd.c = e.l_cmd.c;
    LLaserCmd.d = e.l_cmd.d;
    LLaserCmd.dx = e.l_cmd.x;
    LLaserCmd.dy = e.l_cmd.y;
    LLaserCmd.e = &e;
    LLaserCmd.type = e.l_cmd.type;
    LLaserCmd.v = e.l_cmd.v;
    LLaserCmd.w = e.l_cmd.w;
    if (LLaserSet(e.LLaserRef))
      e.LLaserRef++;
    break;
  }
  case EclOp::LLOPEN: {
    auto c = Decode(EclOpTag<EclOp::LLOPEN>{}, raw);
    LLaserOpen(&e, c.id);
    break;
  }
  case EclOp::LLCLOSE: {
    auto c = Decode(EclOpTag<EclOp::LLCLOSE>{}, raw);
    LLaserClose(&e, c.id);
    e.LLaserRef = (c.id == ECLCST_LLASERALL) ? 0 : e.LLaserRef - 1;
    break;
  }
  case EclOp::LLCLOSEL: {
    auto c = Decode(EclOpTag<EclOp::LLCLOSEL>{}, raw);
    LLaserLine(&e, c.id);
    break;
  }
  case EclOp::LLDEGR: {
    auto c = Decode(EclOpTag<EclOp::LLDEGR>{}, raw);
    LLaserDegR(&e, c.deg, c.id);
    break;
  }
  case EclOp::HLASER: {
    HInfo.c = e.l_cmd.c;
    HInfo.d = e.l_cmd.d;
    HInfo.dw = e.l_cmd.dw;
    HInfo.n = e.l_cmd.n;
    HInfo.type = e.l_cmd.type;
    HInfo.x = e.x + e.l_cmd.x;
    HInfo.y = e.y + e.l_cmd.y;
    HLaserSet(&HInfo);
    e.cmd += CmdLength(EclOp::HLASER);
    return; // stop for frame (was implicit bRetFlag in old code)
  }

  // ============================================================
  // 0x9?: Flag toggles
  // ============================================================
  case EclOp::DRAW_ON:
    e.flag |= EF_DRAW;
    break;
  case EclOp::DRAW_OFF:
    e.flag &= ~EF_DRAW;
    break;
  case EclOp::CLIP_ON:
    e.flag |= EF_CLIP;
    break;
  case EclOp::CLIP_OFF:
    e.flag &= ~EF_CLIP;
    break;
  case EclOp::DAMAGE_ON:
    e.flag |= EF_DAMAGE;
    break;
  case EclOp::DAMAGE_OFF:
    e.flag &= ~EF_DAMAGE;
    break;
  case EclOp::HITSB_ON:
    e.flag |= EF_HITSB;
    break;
  case EclOp::HITSB_OFF:
    e.flag &= ~EF_HITSB;
    break;
  case EclOp::RLCHG_ON:
    if (e.x < GX_MID)
      e.flag |= EF_RLCHG;
    else
      e.flag &= ~EF_RLCHG;
    break;
  case EclOp::RLCHG_OFF:
    e.flag &= ~EF_RLCHG;
    break;

  // ============================================================
  // 0xA?: Special commands
  // ============================================================
  case EclOp::ANM: {
    auto c = Decode(EclOpTag<EclOp::ANM>{}, raw);
    e.anm_ptn = e.anm_ptnEx = c.ptn;
    e.anm_sp = c.sp;
    e.g_height = Anime[e.anm_ptn].size.h << 5;
    e.g_width = Anime[e.anm_ptn].size.w << 5;
    e.anm_c = 0;
    break;
  }
  case EclOp::ANMEX: {
    auto c = Decode(EclOpTag<EclOp::ANMEX>{}, raw);
    e.anm_ptnEx = c.ptn;
    break;
  }
  case EclOp::PSE: {
    auto c = Decode(EclOpTag<EclOp::PSE>{}, raw);
    Snd_SEPlay(c.id, e.x);
    break;
  }
  case EclOp::INT: {
    auto c = Decode(EclOpTag<EclOp::INT>{}, raw);
    BossINT(&e, static_cast<EclIntType>(c.id));
    break;
  }
  case EclOp::EXDEGD: {
    auto c = Decode(EclOpTag<EclOp::EXDEGD>{}, raw);
    EnemyEXDEG_D = c.d;
    break;
  }
  case EclOp::ENEMYSET: {
    auto c = Decode(EclOpTag<EclOp::ENEMYSET>{}, raw);
    if (EnemyNow + 1 < ENEMY_MAX) {
      auto *ne = &Enemy[EnemyInd[EnemyNow++]];
      short sx = (e.x >> 6) + c.x;
      short sy = (e.y >> 6) + c.y;
      InitEnemyDataSTD(ne, sx, sy, 4 + (c.ecl_id << 2));
    }
    break;
  }
  case EclOp::ENEMYSETD: {
    auto c = Decode(EclOpTag<EclOp::ENEMYSETD>{}, raw);
    if (EnemyNow + 1 < ENEMY_MAX) {
      auto *ne = &Enemy[EnemyInd[EnemyNow++]];
      short sx = (e.x >> 6) + c.x;
      short sy = (e.y >> 6) + c.y;
      InitEnemyDataSTD(ne, sx, sy, 4 + (c.ecl_id << 2));
      ne->d = ResolveValue(&e, static_cast<EclReg>(c.reg));
    }
    break;
  }
  case EclOp::HITXY: {
    auto c = Decode(EclOpTag<EclOp::HITXY>{}, raw);
    e.g_width = PixelToWorld(c.w);
    e.g_height = PixelToWorld(c.h);
    break;
  }
  case EclOp::ITEM: {
    auto c = Decode(EclOpTag<EclOp::ITEM>{}, raw);
    e.item = c.type;
    e.cmd += CmdLength(EclOp::ITEM);
    return; // stop for frame (was implicit bRetFlag in old code)
  }
  case EclOp::STG4EFC: {
    auto c = Decode(EclOpTag<EclOp::STG4EFC>{}, raw);
    switch (c.cmd) {
    case STG4ROCK_STDMOVE:
    case STG4ROCK_ACCMOVE1:
    case STG4ROCK_3DMOVE:
    case STG4ROCK_LEAVE:
    case STG4ROCK_END:
      SendCmdStg4Rock(c.cmd, 0);
      break;
    case STG4ROCK_ACCMOVE2:
      SendCmdStg4Rock(c.cmd, e.d);
      break;
    }
    break;
  }
  case EclOp::STG3EFC:
    ScrollCommand(SCMD_STG3STAR);
    e.cmd += CmdLength(EclOp::STG3EFC);
    return; // stop for frame

  case EclOp::BITLASER: {
    auto c = Decode(EclOpTag<EclOp::BITLASER>{}, raw);
    BossBitLaser(&e, c.cmd);
    break;
  }
  case EclOp::BITATTACK: {
    auto c = Decode(EclOpTag<EclOp::BITATTACK>{}, raw);
    BossBitAttack(&e, c.atk_id);
    break;
  }
  case EclOp::BITCMD: {
    auto c = Decode(EclOpTag<EclOp::BITCMD>{}, raw);
    BossBitCommand(&e, c.cmd, c.param);
    break;
  }
  case EclOp::BOSSSET: {
    auto c = Decode(EclOpTag<EclOp::BOSSSET>{}, raw);
    BossSetEx(e.x >> 6, e.y >> 6, c.id);
    break;
  }
  case EclOp::CEFC: {
    auto c = Decode(EclOpTag<EclOp::CEFC>{}, raw);
    CEffectSet(e.x + PixelToWorld(c.x), e.y + PixelToWorld(c.y), c.id);
    break;
  }

  // ============================================================
  // 0xB?: Register operations
  // ============================================================
  case EclOp::MOVR: {
    auto c = Decode(EclOpTag<EclOp::MOVR>{}, raw);
    auto val = ResolveValue(&e, static_cast<EclReg>(c.src));
    switch (static_cast<EclReg>(c.dst)) {
    case EclReg::GR0:
    case EclReg::GR1:
    case EclReg::GR2:
    case EclReg::GR3:
    case EclReg::GR4:
    case EclReg::GR5:
    case EclReg::GR6:
    case EclReg::GR7:
      e.GR[std::to_underlying(static_cast<EclReg>(c.dst))] = val;
      break;
    case EclReg::LCMD_D:
      e.l_cmd.d = val;
      break;
    case EclReg::LCMD_DW:
      e.l_cmd.dw = val;
      break;
    case EclReg::LCMD_N:
      e.l_cmd.n = val;
      break;
    case EclReg::LCMD_C:
      e.l_cmd.c = val;
      break;
    case EclReg::LCMD_L:
      e.l_cmd.l = val;
      break;
    case EclReg::LCMD_V:
      e.l_cmd.v = val;
      break;
    case EclReg::TCMD_D:
      e.t_cmd.d = val;
      break;
    case EclReg::TCMD_DW:
      e.t_cmd.dw = val;
      break;
    case EclReg::TCMD_N:
      e.t_cmd.n = val;
      break;
    case EclReg::TCMD_NS:
      e.t_cmd.ns = val;
      break;
    case EclReg::TCMD_V:
      e.t_cmd.v = val;
      break;
    case EclReg::TCMD_C:
      e.t_cmd.c = val;
      break;
    case EclReg::TCMD_A:
      e.t_cmd.a = val;
      break;
    case EclReg::TCMD_REP:
      e.t_cmd.rep = val;
      break;
    case EclReg::TCMD_VD:
      e.t_cmd.vd = val;
      break;
    case EclReg::ENEMY_X:
      e.x = val;
      break;
    case EclReg::ENEMY_Y:
      e.y = val;
      break;
    case EclReg::ENEMY_D:
      e.d = val;
      break;
    default:
      DebugOut(u8"unknown register");
      break;
    }
    break;
  }
  case EclOp::MOVC: {
    auto c = Decode(EclOpTag<EclOp::MOVC>{}, raw);
    if (c.dst < ECLREG_MAX)
      e.GR[c.dst] = c.value;
    else
      DebugOut(u8"unknown register");
    break;
  }
  case EclOp::INC: {
    auto c = Decode(EclOpTag<EclOp::INC>{}, raw);
    if (c.dst < ECLREG_MAX)
      e.GR[c.dst]++;
    break;
  }
  case EclOp::DEC: {
    auto c = Decode(EclOpTag<EclOp::DEC>{}, raw);
    if (c.dst < ECLREG_MAX)
      e.GR[c.dst]--;
    break;
  }
  case EclOp::ADD: {
    auto c = Decode(EclOpTag<EclOp::ADD>{}, raw);
    if (c.dst < ECLREG_MAX)
      e.GR[c.dst] += ResolveValue(&e, static_cast<EclReg>(c.src));
    break;
  }
  case EclOp::SUB: {
    auto c = Decode(EclOpTag<EclOp::SUB>{}, raw);
    if (c.dst < ECLREG_MAX)
      e.GR[c.dst] -= ResolveValue(&e, static_cast<EclReg>(c.src));
    break;
  }
  case EclOp::SINL: {
    auto c = Decode(EclOpTag<EclOp::SINL>{}, raw);
    if (c.dst < ECLREG_MAX && c.src < ECLREG_MAX)
      e.GR[c.dst] = sinl(static_cast<uint8_t>(e.GR[c.src]), e.GR[c.dst]);
    break;
  }
  case EclOp::COSL: {
    auto c = Decode(EclOpTag<EclOp::COSL>{}, raw);
    if (c.dst < ECLREG_MAX && c.src < ECLREG_MAX)
      e.GR[c.dst] = cosl(static_cast<uint8_t>(e.GR[c.src]), e.GR[c.dst]);
    break;
  }
  case EclOp::MOD: {
    auto c = Decode(EclOpTag<EclOp::MOD>{}, raw);
    if (c.dst < ECLREG_MAX && c.value != 0)
      e.GR[c.dst] %= c.value;
    break;
  }
  case EclOp::RND: {
    auto c = Decode(EclOpTag<EclOp::RND>{}, raw);
    if (c.dst < ECLREG_MAX)
      e.GR[c.dst] = static_cast<uint32_t>(rnd()) * rnd();
    break;
  }
  case EclOp::CMPR: {
    auto c = Decode(EclOpTag<EclOp::CMPR>{}, raw);
    if (c.reg0 < ECLREG_MAX && c.reg1 < ECLREG_MAX)
      RegCmp = ResolveValue(&e, static_cast<EclReg>(c.reg0)) -
               ResolveValue(&e, static_cast<EclReg>(c.reg1));
    else
      return;
    break;
  }
  case EclOp::CMPC: {
    auto c = Decode(EclOpTag<EclOp::CMPC>{}, raw);
    if (c.reg < ECLREG_MAX)
      RegCmp = ResolveValue(&e, static_cast<EclReg>(c.reg)) -
               static_cast<int>(c.value);
    else
      return;
    break;
  }
  case EclOp::JL: {
    auto c = Decode(EclOpTag<EclOp::JL>{}, raw);
    if (RegCmp > 0) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::JS: {
    auto c = Decode(EclOpTag<EclOp::JS>{}, raw);
    if (RegCmp < 0) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }
  case EclOp::JEQ: {
    auto c = Decode(EclOpTag<EclOp::JEQ>{}, raw);
    if (RegCmp == 0) {
      e.cmd = c.target;
      goto ECL_HEAD;
    }
    break;
  }

  default:
    return;
  }

  e.cmd += CmdLength(op);
  goto ECL_HEAD;
}

// 割り込みジャンプを調べる //
void EclVM::CheckInterrupts(EnemyData &e_ref) {
  auto *e = &e_ref;

  for (int i = 0; i < ECLVECT_MAX; i++) {
    if (e->Vect[i].vect == 0)
      continue; // 割り込みがかかっていない
    switch (static_cast<EclIntVec>(i)) {
    case (EclIntVec::BITLEFT): // ビット残り割り込み
      if (BossGetBitLeft() <=
          e->Vect[std::to_underlying(EclIntVec::BITLEFT)].value) {
        e->cmd = e->Vect[std::to_underlying(EclIntVec::BITLEFT)].vect;
        e->cmd_c = 0; // コマンド繰り返しカウンタ
        e->rep_c = 0; // LOOP(旧REP)命令カウンタ
        e->t_rep = 0; // 自動弾発射タイミング(0:自動発射せず)
        return;
      }
      break;

    case (EclIntVec::BOSSLEFT): // ボス残り割り込み
      if (BossNow <= e->Vect[std::to_underlying(EclIntVec::BOSSLEFT)].value) {
        e->cmd = e->Vect[std::to_underlying(EclIntVec::BOSSLEFT)].vect;
        e->cmd_c = 0; // コマンド繰り返しカウンタ
        e->rep_c = 0; // LOOP(旧REP)命令カウンタ
        e->t_rep = 0; // 自動弾発射タイミング(0:自動発射せず)
        return;
      }
      break;

    case (EclIntVec::HP): // HPL 割り込み
      if (e->hp <= e->Vect[std::to_underlying(EclIntVec::HP)].value) {
        e->cmd = e->Vect[std::to_underlying(EclIntVec::HP)].vect;
        e->cmd_c = 0; // コマンド繰り返しカウンタ
        e->rep_c = 0; // LOOP(旧REP)命令カウンタ
        e->t_rep = 0; // 自動弾発射タイミング(0:自動発射せず)
        return;
      }
      break;

    case (EclIntVec::TIMER): // タイマー割り込み
      if (e->IntTimer > e->Vect[std::to_underlying(EclIntVec::TIMER)].value) {
        e->cmd = e->Vect[std::to_underlying(EclIntVec::TIMER)].vect;
        e->cmd_c = 0;    // コマンド繰り返しカウンタ
        e->rep_c = 0;    // LOOP(旧REP)命令カウンタ
        e->t_rep = 0;    // 自動弾発射タイミング(0:自動発射せず)
        e->IntTimer = 0; // この割り込み特有の初期化
        return;
      } else {
        e->IntTimer++;
      }
      break;

    default:
      break;
    }
  }
}

// 割り込みベクタの初期化 //
void EclVM::InitInterrupts(EnemyData &e_ref) {
  auto *e = &e_ref;
  for (auto &it : e->Vect) {
    it.vect = 0;
  }
}

// 強制的に ECL ブロック間を移動する //
void EclVM::LongJump(EnemyData &e_ref, uint32_t ecl_id) {
  auto *e = &e_ref;
  e->cmd = U32LEAt(&m_ecl_data[ecl_id]);
  e->call_addr = e->cmd;
  e->t_rep = 0; // 弾の発射間隔(０：自動発射しない)
  e->rep_c = 0;
  e->cmd_c = 0;
}
