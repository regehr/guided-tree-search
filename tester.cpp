#include <cassert>
#include <iostream>
#include <vector>

#include "generator.h"

/*
 * test tree generators
 *
 * when called, each of these functions uses the supplied generator to
 * walk a single path through the decision tree that it describes.
 * the return value is the number of the leaf that was reached by that
 * traversal.
 *
 * the painful part of this interface is that leaf numbers should be
 * fairly dense, ideally perfectly dense in range 0 .. NUM_LEAVES-1.
 * so you have to map leaves to leaf numbers yourself -- an automated
 * numbering scheme does not seem possible without fully enumerating
 * the leaves (which won't be possible for many trees of interest)
 */

/*
 * maximally unbalanced n-ary tree
 */
static unsigned long test1_helper(uniform::Generator &G, int Depth, int Number,
                                  int BranchFactor) {
  if (Depth == 0)
    return Number;
  auto Choice = G.choose(BranchFactor);
  if (Choice != (BranchFactor - 1))
    return Number + Choice;
  return test1_helper(G, Depth - 1, Number + BranchFactor - 1, BranchFactor);
}

static unsigned long test1(uniform::Generator &G) {
  const int TreeDepth = 3;
  const int BranchFactor = 10;

  return test1_helper(G, TreeDepth, 0, BranchFactor);
}

/*
 * full tree
 */
static unsigned long test2_helper(uniform::Generator &G, int Depth, int Number,
                                  int BranchFactor) {
  if (Depth == 0) {
    return Number;
  } else {
    return test2_helper(G, Depth - 1,
                        (BranchFactor * Number) + G.choose(BranchFactor),
                        BranchFactor);
  }
}

static unsigned long test2(uniform::Generator &G) {
  const int TreeDepth = 6;
  const int BranchFactor = 2;

  return test2_helper(G, TreeDepth, 0, BranchFactor);
}

/*
 * TODO heavily skewed tree
 */

/*
 * TODO somewhat unstructured tree
 */

int main() {
  const int REPS = 100 * 1000;
  std::vector<int> Results;
  uniform::Generator G;

  std::vector<unsigned long (*)(uniform::Generator &)> TreeGenerators = {test1,
                                                                         test2};

  for (int rep = 0; rep < REPS; ++rep) {
    if (!G.start())
      break;
    auto Res = test1(G);
    std::cout << "Res = " << Res << "\n";
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
