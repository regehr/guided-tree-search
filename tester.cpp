#include <cassert>
#include <iostream>
#include <vector>

#include "generator.h"

int main() {
  const int REPS = 100000;
  const int TreeDepth = 10;

  std::vector<int> results(TreeDepth + 1);
  uniform::Generator g;

  for (int rep = 0; rep < REPS; ++rep) {
    g.start();
    for (int i = 0; i < TreeDepth; ++i) {
      if (g.flip()) {
        results[i]++;
        goto done;
      }
    }
    results[TreeDepth]++;
  done:;
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
