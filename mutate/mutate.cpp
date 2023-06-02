#include "mutate.h"

using namespace tree_guide;

namespace mutator {

/////////////////////////////////////////////////////////////////////////////////////
// TODO
// - lots more work on scope-aware mutations

static std::unique_ptr<std::mt19937_64> Rand;

void init(long Seed) { Rand = std::make_unique<std::mt19937_64>(Seed); }

static void change_one(std::vector<rec> &C) {
  std::uniform_int_distribution<uint64_t> FullDist(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  std::uniform_int_distribution<uint64_t> LimitedDist(0, C.size() - 1);
  uint64_t x;
  // this could perform poorly if a choice sequence is almost entirely
  // scope changes, or loop infinitely if there aren't any numeric
  // choices, we'll deal with that when it happens
  do {
    x = LimitedDist(*Rand.get());
  } while (C.at(x).k != RecKind::NUM);
  C.at(x).v = FullDist(*Rand.get());
}

void mutate_choices(std::vector<rec> &C) {
  std::uniform_int_distribution<uint64_t> CoinDist(0, 1);
  do {
    change_one(C);
  } while (CoinDist(*Rand.get()) == 0);
}

} // end namespace mutator

