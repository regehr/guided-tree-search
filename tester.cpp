#include <cassert>
#include <iostream>
#include <vector>

#include "generator.h"

static const int TreeDepth = 10;
static std::vector<int> results(TreeDepth + 1);
static uniform::Generator g;

static int test() {
  for (int i = 0; i < TreeDepth; ++i) {
    if (g.flip())
      return i;
  }
  return TreeDepth;
}

int main() {
  const int REPS = 100000;

  for (int rep = 0; rep < REPS; ++rep) {
    if (!g.start())
      break;
    auto res = test();
    results[res]++;
  }

  int total = 0;
  for (int i = 0; i <= TreeDepth; i++) {
    std::cout << i << " : " << results[i] << "\n";
    total += results[i];
  }
  std::cout << "total = " << total << "\n";
  assert(total == REPS);

  std::cout << "Done.\n";
  return 0;
}
