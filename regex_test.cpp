#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "regex.h"

const long N = 1000*1000*1000;
const long Depth = 4;

int main() {
  tree_guide::BFSGuide G;
  for (int i = 0; i < N; ++i) {
    auto C = G.makeChooser();
    if (!C) {
      std::cout << "*** tree fully explored ***\n";
      break;
    }
    auto Str = gen(*C, Depth);
    if (false) {
      auto Ret = system(("grep '" + Str + "' < ../regex.cpp >/dev/null").c_str());
      if (Ret != 0 && Ret != 256)
        std::cout << Ret << " : " << Str << "\n";
    }
  }
  return 0;
}

