#include "regex.h"

//////////////////////////////////////////////////////////////////////////////

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

static std::string gen_helper(tree_guide::Chooser &C, long Depth) {
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
  case 10: {
    auto N = num(C, 0, 5);
    return gen(C, Depth) + "{" + std::to_string(N) + "," +
           std::to_string(N + num(C, 0, 4)) + "}";
  }
  default:
    assert(false);
  }
}

std::string gen(tree_guide::Chooser &C, long Depth) {
  C.beginScope();
  auto s = gen_helper(C, Depth);
  C.endScope();
  return s;
}

//////////////////////////////////////////////////////////////////////////////
