#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "gen_regex.h"

const long N = 250 * 1000;

const bool RUN_GREP = false;

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
    auto Str = gen(*C, RegexDepth);
    Results.emplace(Str);
    if (RUN_GREP) {
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
    DefaultGuide G1;
    SaverGuide G2(&G1, "// ");
    go(G2);
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
    auto G1 = new DefaultGuide();
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
