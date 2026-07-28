/// Score scene and persistence.

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstring>
#include <format>
#include <ranges>
#include <utility>

#include "game_main.h"
#include "gameflow_manager.h"
#include "score_scene.h"

#include "audio/snd.h"
#include "gameplay/game_rules.h"
#include "gfx/constants.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "sys/input.h"
#include "util/debug.h"
#include "util/guard.h"

constexpr char ScoreFileName[] = "秋霜SC.DAT"; // Score data file name

namespace {
constexpr char kDifficultyNames[5][8] = {"Easy", "Normal", "Hard", "Lunatic",
                                         "Extra"};
} // namespace

GameLevel ScoreScene::CurrentLevel() const {
  return GameFlow.ctx.session.stage == StageId::Extra
             ? GameLevel::Extra
             : GameFlow.ctx.session.level;
}

bool ScoreScene::ShowLeaderboard() {
  current_difficulty_ = std::to_underlying(CurrentLevel());
  current_rank_ =
      BuildRows(nullptr, static_cast<GameLevel>(current_difficulty_));
  if (current_rank_ == 0) {
    return GameExit();
  }

  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();

  if (!GameFlow.ctx.graphics.LoadNameRegistration()) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);
  input_locked_ = Key_Data != 0U;
  GameFlow.game_main = [](bool &q) { GameFlow.ctx.score.UpdateLeaderboard(q); };
  GameFlow.current_state = GameState::Leaderboard;
  return true;
}

void ScoreScene::UpdateLeaderboard(bool & /*unused*/) {
  switch (Key_Data) {
  case KEY_RETURN:
  case KEY_TAMA:
  case KEY_BOMB:
  case KEY_ESC:
    if (input_locked_) {
      break;
    }
    Snd_SEPlay(SfxId::Cancel);
    (void)GameExit(false);
    return;

  case KEY_UP:
  case KEY_LEFT:
    if (rows_.back().moving) {
      break;
    }
    Snd_SEPlay(SfxId::Select);
    current_difficulty_ = (current_difficulty_ + 4) % 5;
    current_rank_ =
        BuildRows(nullptr, static_cast<GameLevel>(current_difficulty_));
    break;

  case KEY_DOWN:
  case KEY_RIGHT:
    if (rows_.back().moving) {
      break;
    }
    Snd_SEPlay(SfxId::Select);
    current_difficulty_ = (current_difficulty_ + 1) % 5;
    current_rank_ =
        BuildRows(nullptr, static_cast<GameLevel>(current_difficulty_));
    break;

  case 0:
    input_locked_ = false;
    break;
  }

  GrpBackend_Clear();
  DrawScores();
  GrpPut16(320, 450, kDifficultyNames[current_difficulty_]);
  Grp_Flip();
}

void ScoreScene::DrawScores() {
  for (std::size_t i = 0; i < rows_.size(); i++) {
    auto &row = rows_[i];
    const auto velocity = (row.x - ((50 + (i * 24)) << 6)) / 12;
    if (velocity > 2_px) {
      row.x -= velocity;
    } else {
      row.moving = false;
    }

    auto src =
        PIXEL_LTRB{PIXEL_LTWH{0, static_cast<int>(64 + (32 * i)), 400, 32}};
    const WINDOW_POINT topleft = {(row.x >> 6), (row.y >> 6)};
    GrpSurface_Blit(topleft, SURFACE_ID::NAMEREG, src);

    auto gx = (row.x >> 6) + 88;
    auto gy = (row.y >> 6) + 4;
    GrpPut16c2(gx, gy, row.name);

    gx = (row.x >> 6) + 232 - 16;
    GrpPut16c2(gx, gy, row.score.c_str());

    gx = (row.x >> 6) + 120;
    gy = (row.y >> 6) + 25;
    GrpPutScore(gx, gy, row.graze.c_str());

    gx = (row.x >> 6) + 224;
    if (row.stage[0] == '7') {
      src = {288, 88, (288 + 16), (88 + 8)};
      GrpSurface_Blit({gx, (gy - 1)}, SURFACE_ID::SYSTEM, src);
    } else {
      GrpPutScore(gx, gy, row.stage.c_str());
    }

    gx = (row.x >> 6) + 224 + 80;
    src = PIXEL_LTWH{0, (400 + (row.weapon * 8)), 48, 8};
    GrpSurface_Blit({gx, (gy - 1)}, SURFACE_ID::NAMEREG, src);
  }
}

bool ScoreScene::StartNameRegistration(bool change_music) {
  current_entry_ = {};
  current_entry_.score = GameFlow.ctx.player.Score();
  current_entry_.graze = GameFlow.ctx.player.GrazeSum();
  current_entry_.weapon = std::to_underlying(GameFlow.ctx.player.Type());
  current_entry_.stage =
      GameFlow.ctx.session.stage == StageId::Extra
          ? 1
          : std::to_underlying(GameFlow.ctx.session.stage) + 1;

  Snd_SEStop(8);
  Snd_SEStopAll();

  current_rank_ = BuildRows(&current_entry_, CurrentLevel());
  if (current_rank_ == 0) {
    return GameExit();
  }

  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();

  if (!GameFlow.ctx.graphics.LoadNameRegistration()) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);
  name_entry_.Begin(false);
  GameFlow.game_main = [](bool &q) {
    GameFlow.ctx.score.UpdateNameRegistration(q);
  };
  GameFlow.current_state = GameState::NameRegistration;

  if (change_music) {
    GameFlow.ctx.music.Play(19);
  }
  return true;
}

void ScoreScene::UpdateNameRegistration(bool & /*unused*/) {
  const auto result = name_entry_.Update(Key_Data);
  auto &name = rows_[current_rank_ - 1].name;
  std::ranges::fill(name, '\0');
  const auto entered_name = name_entry_.Name();
  std::ranges::copy(entered_name, name);

  if (result == NameEntryResult::Confirmed) {
    std::copy_n(name, kScoreNameLength, current_entry_.name);
    (void)Save(current_entry_, CurrentLevel());
    (void)GameExit();
    return;
  }

  GrpBackend_Clear();
  GrpGeom->Lock();
  GrpGeom->SetColor({2, 0, 0});
  auto gx = rows_[current_rank_ - 1].x >> 6;
  auto gy = rows_[current_rank_ - 1].y >> 6;
  GrpGeom->DrawBox(gx, gy, (gx + 400), (gy + 32));

  GrpGeom->Unlock();

  DrawScores();
  name_entry_.Draw(gx + 88, gy + 4);
  Grp_Flip();
}

// Get current score string (with name insertion)
// If NData == NULL, no insertion
uint8_t ScoreScene::BuildRows(ScoreEntry *NData, GameLevel Dif) {
  ScoreRow *Res = nullptr;
  int i = 0;
  int num = 0;
  int64_t temp = 0;

  Res = rows_.data();

  // Load score data
  uint8_t rank = 0;
  if (NData != nullptr) {
    rank = RankOf(*NData, Dif);
    if (rank == 0) {
      return 0;
    }
  } else {
    rank = kScoreRankCount;
  }

  // Set the pointer
  if (!Load()) {
    return 0;
  }
  auto maybe_p = ScoresFor(Dif);
  if (!maybe_p) {
    Release();
    return 0;
  }
  auto p = maybe_p.value();

  if ((rank != 0) && (NData != nullptr)) {
    // Shift scores down first
    for (i = kScoreRankCount - 1; std::cmp_greater_equal(i, rank); i--) {
      p[i] = p[i - 1]; // struct assignment
    }

    // Insert new data
    p[rank - 1] = *NData;
  }

  // Start data storage
  temp = num = 0;
  for (i = 0; std::cmp_less(i, kScoreRankCount); i++) {
    if (temp < p[i].score) {
      temp = p[i].score;
      num += 1;
    }

    Res[i].rank = num;
    Res[i].x = (640 + (50 + (i * 24 * 20))) << 6;
    Res[i].y = (100 + (i * 48)) << 6;
    Res[i].moving = true;

    const auto name_end = std::ranges::find(p[i].name, '\0');
    const auto name_length = static_cast<std::size_t>(name_end - p[i].name);
    const auto copied = std::min(name_length, kScoreNameLength - 1);
    std::ranges::fill(Res[i].name, '\0');
    std::copy_n(p[i].name, copied, Res[i].name);

    Res[i].weapon = p[i].weapon % 4;

    Res[i].score = std::format("{:11}", p[i].score);
    Res[i].graze = std::format("{:6}", p[i].graze);
    Res[i].stage = std::format("{:1}", p[i].stage);
  }

  Release();

  return rank;
}

uint8_t ScoreScene::RankOf(const ScoreEntry &NData, GameLevel Dif) {
  // Cannot load, fail
  if (!Load()) {
    return 0;
  }
  auto score_guard = make_guard([this] { Release(); });

  // Assign pointer by difficulty
  const auto maybe_temp = ScoresFor(Dif);
  if (!maybe_temp) {
    return 0;
  }
  const auto temp = maybe_temp.value();

  // Find matching slot
  for (const auto Rank : std::views::iota(0U, temp.size())) {
    // Equal scores go to lower rank
    // (Same as previous game [Shushoku(Temporary])
    if (NData.score > temp[Rank].score) {
      // Return rank
      return (Rank + 1);
    }
  }
  return 0;
}

// Write score data
bool ScoreScene::Save(const ScoreEntry &NData, GameLevel Dif) {
  // Load score data
  const auto Rank = RankOf(NData, Dif);

  // Not a high score
  if (Rank == 0) {
    return false;
  }

  // Set the pointer
  if (!Load()) {
    return false;
  }
  auto maybe_temp = ScoresFor(Dif);
  if (!maybe_temp) {
    Release();
    return false;
  }
  auto temp = maybe_temp.value();

  // Shift scores down first
  for (auto i = (temp.size() - 1); i >= Rank; i--) {
    temp[i] = temp[i - 1]; // struct assignment
  }

  // Insert new data
  temp[Rank - 1] = NData;

  // Write to file
  BitWriter bd;
  SaveList(score_cache_->easy, bd);
  SaveList(score_cache_->normal, bd);
  SaveList(score_cache_->hard, bd);
  SaveList(score_cache_->lunatic, bd);
  SaveList(score_cache_->extra, bd);
  Release();

  return bd.Save(ScoreFileName);
}

// Load score data
bool ScoreScene::Load() {
  bool bInit = false;

  // Already loaded (not a failure)
  if (score_cache_) {
    return true;
  }

  score_cache_ = std::make_unique<ScoreData>();
  if (score_cache_ == nullptr) {
    return false;
  }

  // Open file in bit read mode
  auto bd = LoadBitFile(ScoreFileName);
  while (1) {
    if (!LoadList(score_cache_->easy, bd)) {
      break;
    }
    if (!LoadList(score_cache_->normal, bd)) {
      break;
    }
    if (!LoadList(score_cache_->hard, bd)) {
      break;
    }
    if (!LoadList(score_cache_->lunatic, bd)) {
      break;
    }
    if (!LoadList(score_cache_->extra, bd)) {
      break;
    }

    bInit = true;
    break;
  }

  if (!bInit) {
    // If file does not exist or is invalid, create new one
    // No write to file at this point
    return SetDefaults();
  }

  return true;
}

void ScoreScene::Release() {
  // Release
  score_cache_ = nullptr;
}

// Assign pointer by difficulty
std::optional<ScoreScene::ScoreList>
ScoreScene::ScoresFor(GameLevel Dif) const {
  if (!score_cache_) {
    return {};
  }

  switch (Dif) {
  case GameLevel::Easy:
    return score_cache_->easy;
  case GameLevel::Normal:
    return score_cache_->normal;
  case GameLevel::Hard:
    return score_cache_->hard;
  case GameLevel::Lunatic:
    return score_cache_->lunatic;
  case GameLevel::Extra:
    return score_cache_->extra;
  default:
    return {};
  }
}

// Set default score data
bool ScoreScene::SetDefaults() {
  if (nullptr == score_cache_) {
    return false;
  }

  for (auto i = 0; i < std::to_underlying(GameLevel::Extra) + 1; i++) {
    auto maybe_temp = ScoresFor(static_cast<GameLevel>(i));
    if (!maybe_temp) {
      return false;
    }
    auto temp = maybe_temp.value();
    for (size_t j = 0; j < temp.size(); j++) {
      std::copy_n("????????", kScoreNameLength, temp[j].name);
      temp[j].score = ((temp.size() - j) * uint64_t{1200000});
      temp[j].graze = ((temp.size() - j) * 50);
      temp[j].stage = ((i < 4) ? (temp.size() - j) : 1);
      temp[j].weapon = j % 3;
    }
  }

  return true;
}

bool ScoreScene::LoadList(ScoreList NData, BitReader &bd) {
  uint64_t CheckSum = 0;
  uint64_t Mask = kScoreMask;
  uint8_t flag = 0;

  for (auto &nd : NData) {
    CheckSum = 0;
    if (flag != bd.ReadBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire name
    for (auto &c : nd.name) {
      c = ReadMasked<uint8_t>(bd, Mask);
      CheckSum += c;
    }
    if (flag != bd.ReadBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire score
    nd.score = ReadMasked<uint64_t>(bd, Mask);
    CheckSum += nd.score;
    if (flag != bd.ReadBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire graze
    nd.graze = ReadMasked<uint32_t>(bd, Mask);
    CheckSum += nd.graze;
    if (flag != bd.ReadBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire stage
    nd.stage = ReadMasked<uint8_t>(bd, Mask);
    CheckSum += nd.stage;
    if (flag != bd.ReadBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire weapon
    nd.weapon = ReadMasked<uint8_t>(bd, Mask);
    CheckSum += nd.weapon;
    if (flag != bd.ReadBit()) {
      return false;
    }
    flag = 1 - flag;

    // Checksum comparison
    if (CheckSum != ReadMasked<uint64_t>(bd, Mask)) {
      return false;
    }
  }

  return true;
}

void ScoreScene::SaveList(ConstScoreList NData, BitWriter &bd) {
  uint64_t CheckSum = 0;
  uint64_t Mask = kScoreMask;
  uint8_t flag = 0;

  for (const auto &nd : NData) {
    CheckSum = 0;
    bd.WriteBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output name
    for (const auto &c : nd.name) {
      CheckSum += c;
      WriteMasked(bd, static_cast<unsigned char>(c), Mask);
    }
    bd.WriteBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output score
    CheckSum += nd.score;
    WriteMasked(bd, static_cast<uint64_t>(nd.score), Mask);
    bd.WriteBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output graze
    CheckSum += nd.graze;
    WriteMasked(bd, nd.graze, Mask);
    bd.WriteBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output stage
    CheckSum += nd.stage;
    WriteMasked(bd, nd.stage, Mask);
    bd.WriteBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output weapon
    CheckSum += nd.weapon;
    WriteMasked(bd, nd.weapon, Mask);
    bd.WriteBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output checksum
    WriteMasked(bd, CheckSum, Mask);
  }
}
