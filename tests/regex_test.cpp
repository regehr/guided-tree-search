#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "regex.h"

const long N = 1 * 1000 * 1000;
const long Depth = 6;

using namespace std;
using namespace tree_guide;

void go(Guide &G) {
  unordered_set<string> Results;
  for (int i = 0; i < N; ++i) {
    auto C = G.makeChooser();
    if (!C) {
      cout << "*** tree fully explored ***\n";
      break;
    }
    auto Str = gen(*C, Depth);
    Results.emplace(Str);
    if (false) {
      auto Ret =
          system(("grep '" + Str + "' < ../regex.cpp >/dev/null").c_str());
      if (Ret != 0 && Ret != 256)
        cout << Ret << " : " << Str << "\n";
    }
  }
  cout << G.name() << " guide explored " << Results.size() << " leaves\n";
}

int main() {
  cout << "for " << N << " tests:\n";
  {
    SaverGuide<DefaultGuide> G;
    go(G);
  }
  {
    BFSGuide G;
    go(G);
  }
  {
    WeightedSamplerGuide G;
    go(G);
  }
  {
    auto G1 = new SaverGuide<DefaultGuide>();
    auto G2 = new BFSGuide();
    auto G3 = new WeightedSamplerGuide();
    RRGuide G4({G1, G2, G3});
    go(G4);
    delete G1;
    delete G2;
    delete G3;
  }
  return 0;
}
