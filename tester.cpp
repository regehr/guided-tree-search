#include <cassert>
#include <iostream>
#include <vector>

#include "generator.h"

static const int TreeDepth = 55;
static uniform::Generator g;

static unsigned long test1() {
  for (int i = 0; i < TreeDepth; ++i) {
    if (g.flip())
      return i;
  }
  return TreeDepth;
}

static unsigned long test2_helper(int Depth) {
  if (Depth == 0)
    ;
  else
    return test2_helper(Depth - 1);
}

static unsigned long test2() {
  return test2_helper(TreeDepth);
}

int main() {
  const int REPS = 100 * 1000;
  std::vector<int> Results;

  for (int rep = 0; rep < REPS; ++rep) {
    if (!g.start())
      break;
    auto Res = test1();
    if (Res >= Results.size())
      Results.resize(Res + 1);
    ++Results.at(Res);
  }

  int total = 0;
  for (unsigned long i = 0; i < Results.size(); ++i) {
    std::cout << i << " : " << Results.at(i) << "\n";
    total += Results.at(i);
  }
  std::cout << "total = " << total << "\n";
  assert(total == REPS);

  std::cout << "Done.\n";
  return 0;
}
