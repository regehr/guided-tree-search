#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "regex.h"

const long N = 10 * 1000 * 1000;
const long Depth = 15;

using namespace std;
using namespace tree_guide;

void saver_test() {
  SaverGuide<DefaultGuide> G;
  unordered_set<string> Results;
  for (int i = 0; i < N; ++i) {
    auto C1 = G.makeChooser();
    auto C2 = static_cast<SaverChooser<DefaultGuide> *>(C1.get());
    if (!C2) {
      cout << "*** tree fully explored ***\n";
      break;
    }
    auto Str = gen(*C2, Depth);
    if ((i % 1000000) == 0) {
      cout << Str << "\n\n";
      cout << C2->formatChoices();
      cout << "\n";
    }
  }
  cout << G.name() << " guide explored " << Results.size() << " leaves\n";
}

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
  saver_test();
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
