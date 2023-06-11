#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "guide.h"

#include "gen_regex.h"
#include "mutate.h"

using namespace std;
using namespace tree_guide;

static const long N = 10000;
static const long MaxDepth = 10;
// const bool VERBOSE = false;
static const string Prefix("# ");

static std::string::size_type LevDist(const std::string &src, const std::string &tgt) {
  if (src.size() > tgt.size())
    return LevDist(tgt, src);

  using TSizeType = typename std::string::size_type;
  const TSizeType min_size = src.size(), max_size = tgt.size();
  std::vector<TSizeType> lev_dist(min_size + 1);

  for (TSizeType i = 0; i <= min_size; ++i)
    lev_dist[i] = i;

  for (TSizeType j = 1; j <= max_size; ++j) {
    TSizeType prev_diag = lev_dist[0], prev_diag_save;
    ++lev_dist[0];

    for (TSizeType i = 1; i <= min_size; ++i) {
      prev_diag_save = lev_dist[i];
      if (src[i - 1] == tgt[j - 1]) {
        lev_dist[i] = prev_diag;
      } else {
        lev_dist[i] =
            std::min(std::min(lev_dist[i - 1], lev_dist[i]), prev_diag) + 1;
      }
      prev_diag = prev_diag_save;
    }
  }

  return lev_dist[min_size];
}

static void go1(Sync S) {
  double total_dist = 0;
  for (int i = 0; i < N; ++i) {
    DefaultGuide DG;
    SaverGuide SG(&DG, Prefix);
    long Depth = 1 + (i % MaxDepth);
    auto C1 = SG.makeChooser();
    assert(C1);
    auto SC = static_cast<SaverChooser *>(C1.get());
    assert(SC);
    auto OrigStr = gen(*SC, Depth);

    auto OrigChoices = SC->getChoices();
    auto MutChoices = OrigChoices;
    mutator::mutate_choices(MutChoices);

    FileGuide FG;
    FG.setSync(S);
    FG.replaceChoices(MutChoices);
    auto FC = FG.makeChooser();
    auto MutStr = gen(*FC, Depth);
    auto d = LevDist(OrigStr, MutStr);
    total_dist += d;
  }
  cout << "average distance = " << (total_dist / N) << "\n";
}

static void go2() {
  double total_dist = 0;
  for (int i = 0; i < N; ++i) {
    DefaultGuide DG;
    long Depth = 1 + (i % MaxDepth);
    auto C = DG.makeChooser();
    assert(C);
    auto t1 = gen(*C, Depth);
    auto t2 = gen(*C, Depth);
    auto d = LevDist(t1, t2);
    total_dist += d;
  }
  cout << "average distance = " << (total_dist / N) << "\n";
}

int main() {
  for (int i=0; i<5; ++i) {
    go2();
  }
  for (int i=0; i<5; ++i) {
    go1(Sync::NONE);
  }
  for (int i=0; i<5; ++i) {
    go1(Sync::RESYNC);
  }
}
