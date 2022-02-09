#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "guide.h"

static std::string _char(tree_guide::Chooser &C) {
  switch (C.choose(5)) {
  case 0:
    return "a";
  case 1:
    return "b";
  case 2:
    return "c";
  case 3:
    return "d";
  case 4:
    return ".";
  default:
    assert(false);
  }
}

static long num(tree_guide::Chooser &C, long min, long max) {
  return min + C.choose(max - min);
}

static std::string gen(tree_guide::Chooser &C, long Depth) {
  --Depth;
  if (Depth == 0)
    return _char(C);
  switch (C.choose(11)) {
  case 0:
    return _char(C);
  case 1:
    return gen(C, Depth) + "|" + gen(C, Depth);
  case 2:
    return "(" + gen(C, Depth) + ")";
  case 3:
    return gen(C, Depth) + gen(C, Depth);
  case 4:
    return gen(C, Depth) + "?";
  case 5:
    return gen(C, Depth) + "*";
  case 6:
    return gen(C, Depth) + "+";
  case 7:
    return gen(C, Depth) + "{" + std::to_string(num(C, 1, 5)) + "}";
  case 8:
    return gen(C, Depth) + "{" + std::to_string(num(C, 1, 5)) + ",}";
  case 9:
    return gen(C, Depth) + "{," + std::to_string(num(C, 1, 5)) + "}";
  case 10:
    {
      auto N = num(C, 0, 5);
      return gen(C, Depth) + "{" + std::to_string(N) + "," +
        std::to_string(N + num(C, 0, 4)) + "}";
    }
  default:
    assert(false);
  }
}

const long N = 10000;
const long Depth = 3;

int main() {
  tree_guide::BFSGuide G;
  for (int i = 0; i < N; ++i) {
    auto C = G.makeChooser();
    if (!C) {
      std::cout << "*** tree fully explored ***\n";
      break;
    }
    auto Str = gen(*C, Depth);
    auto Ret = system(("grep '" + Str + "' < ../regex.cpp >/dev/null").c_str());
    if (Ret != 0 && Ret != 256)
      std::cout << Ret << " : " << Str << "\n";
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
