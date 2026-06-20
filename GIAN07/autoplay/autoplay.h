#pragma once

#include "game/input.h"
#include <array>
#include <cstdint>
#include <vector>

class AutoPlayController {
public:
  static constexpr int DIFFICULTY_EASY = 0;
  static constexpr int DIFFICULTY_NORMAL = 1;
  static constexpr int DIFFICULTY_HARD = 2;

  static constexpr int PREDICT_FRAMES = 6;
  static constexpr int NUM_CANDIDATES = 9;

  static constexpr int SAFETY_MARGIN_X = 256;
  static constexpr int SAFETY_MARGIN_Y = 256;
  static constexpr int COLLISION_X = (2 * 64) + SAFETY_MARGIN_X;
  static constexpr int COLLISION_Y = (4 * 64) + SAFETY_MARGIN_Y;

  void SetDifficulty(int level);

  INPUT_BITS Update();

private:
  struct BulletPrediction {
    int x[PREDICT_FRAMES];
    int y[PREDICT_FRAMES];
  };

  struct CandidateResult {
    int safe_frames;
    int index;
  };

  struct TargetPoint {
    int x;
    int y;
    bool valid;
  };

  int GetPredictFrames() const;
  int GetDangerThreshold() const;
  int GetBombRadius() const;
  int GetBombThreshold() const;
  int GetGrazeRange() const;

  int GetPlayerSpeed(bool focused) const;
  static int GetStepX(int dir, int speed);
  static int GetStepY(int dir, int speed);

  void BuildPredictions(std::vector<BulletPrediction> &predictions);
  int EvaluateCandidate(const std::vector<BulletPrediction> &predictions,
                        int dir, int player_x, int player_y, int speed);
  int FindBestDirection(const std::vector<BulletPrediction> &predictions,
                        int player_x, int player_y, int speed,
                        int &best_score);

  TargetPoint FindNearestEnemy();
  int DirectionToward(int player_x, int player_y, int target_x, int target_y);

  bool ShouldFocus(int best_score);
  bool ShouldBomb(const std::vector<BulletPrediction> &predictions,
                  int player_x, int player_y, int speed);
  void SteerTowardItems(INPUT_BITS &keys);
  static INPUT_BITS DirectionToKeys(int dx, int dy);

  int difficulty_ = DIFFICULTY_NORMAL;
  int prev_dir_ = 0;
  uint8_t frame_counter_ = 0;
  bool bomb_this_frame_ = false;
};

extern AutoPlayController AutoPlay;
