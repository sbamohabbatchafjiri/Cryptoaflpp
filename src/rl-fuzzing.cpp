#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include <boost/random.hpp>

#include "afl-fuzz-rl.h"
#include "types.h"

using namespace boost;

namespace {
static std::vector<float> computeScores(const rl_params_t *RLParams,
                                        int UseCorrectionFactor) {
  // UseCorrectionFactor: int
      // 0 - For no correction factor
      // 1 - For correction without square root
      // 2 - For correction with square root
  const auto  MapSize = RLParams->map_size;
  const auto *PosRewards = RLParams->positive_reward;
  const auto *NegRewards = RLParams->negative_reward;

  std::vector<float> Scores(MapSize);
#ifdef _DEBUG
  // Fix RNG seed in debug mode
  random::mt19937 RNG;
#else
  random::mt19937 RNG(std::time(nullptr));
#endif

  for (unsigned I = 0; I < MapSize; ++I) {
    random::beta_distribution<> Dist(PosRewards[I], NegRewards[I]);
    Scores[I] = Dist(RNG);
  }
  assert(Scores.size() == MapSize);

   if (UseCorrectionFactor == 1) {
    for (unsigned I = 0; I < MapSize; ++I) {
      const auto PosReward = static_cast<float>(PosRewards[I]);
      const auto NegReward = static_cast<float>(NegRewards[I]);

      const auto Rareness =
          (PosReward + NegReward) /
              (std::pow(PosReward, 2) + PosReward + NegReward);
      Scores[I] *= Rareness;
    } else if (UseCorrectionFactor == 2) {
    for (unsigned I = 0; I < MapSize; ++I) {
      const auto PosReward = static_cast<float>(PosRewards[I]);
      const auto NegReward = static_cast<float>(NegRewards[I]);

      const auto Rareness =
          std::pow((PosReward + NegReward) /
                       (std::pow(PosReward, 2) + PosReward + NegReward),
                   0.5);
      Scores[I] *= Rareness;
    } 
  assert(Scores.size() == MapSize);

  return Scores;
}

static u32 SelectBestBit(const rl_params_t *RLParams,
                          bool               UseCorrectionFactor) {
  const auto &Scores = computeScores(RLParams, UseCorrectionFactor);
  const auto  ArgMax = std::max_element(Scores.begin(), Scores.end());
  return std::distance(Scores.begin(), ArgMax);
}
}  // anonymous namespace

extern "C" u32 rl_select_best_bit(const rl_params_t *rl_params,
                                   bool               UseCorrectionFactor) {
  return SelectBestBit(rl_params, UseCorrectionFactor);
}
