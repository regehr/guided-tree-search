#include <cassert>
#include <iostream>
#include <vector>

#include "generator.h"

int main() {
  const int REPS = 1000;
  const int N = 25;

  std::vector<int> results;
  uniform::Generator g;

  for (int rep = 0; rep < REPS; ++rep) {
    g.start();
    for (int i = 0; i < N; ++i) {
      if (g.flip()) {
        results[i]++;
        break;
      }
      if (i == N-1) {
        results[N]++;
        break;
      }
    }
    assert(false);
  }

  // print stats
  
  std::cout << "Done.\n";
  return 0;
}
