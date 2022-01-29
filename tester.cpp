#include <cassert>
#include <iostream>
#include <string>
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
 * tree where degree increases by one every time we descend a level
 */

/*
 * tree where degree decreases by one every time we descend a level
 */

/*
 * maximally unbalanced n-ary tree
 */
static unsigned long test_maximally_unbalanced_helper(uniform::Generator &G,
                                                      int Depth, int Number,
                                                      int BranchFactor) {
  if (Depth == 0)
    return Number;
  auto Choice = G.choose(BranchFactor);
  if (Choice != (BranchFactor - 1))
    return Number + Choice;
  return test_maximally_unbalanced_helper(
      G, Depth - 1, Number + BranchFactor - 1, BranchFactor);
}

static unsigned long test_maximally_unbalanced(uniform::Generator &G) {
  const int TreeDepth = 5;
  const int BranchFactor = 17;

  return test_maximally_unbalanced_helper(G, TreeDepth, 0, BranchFactor);
}

/*
 * full tree
 */
static unsigned long test_full_tree_helper(uniform::Generator &G, int Depth,
                                           int Number, int BranchFactor) {
  if (Depth == 0) {
    return Number;
  } else {
    return test_full_tree_helper(
        G, Depth - 1, (BranchFactor * Number) + G.choose(BranchFactor),
        BranchFactor);
  }
}

static unsigned long test_full_tree(uniform::Generator &G) {
  const int TreeDepth = 6;
  const int BranchFactor = 2;

  return test_full_tree_helper(G, TreeDepth, 0, BranchFactor);
}

/*
 * Skewed tree: There is a long right-leaning path of depth Depth, and off
 * every point in that path there is a left leaning path that goes to the same
 * depth. The way to hit a leaf is either to go all the way down to the maximum
 * depth, or the go left once and then right at any point after that.
 *
 * A notable feature of this test is that the left branches are much smaller
 * than they look, and the right branches off the main path are quadratically
 * larger than the left branches, so should be picked much more often.
 */

static unsigned long test_right_skewed_tree_left_tree(uniform::Generator &G,
                                                      int Depth, int Number) {
  if (Depth == 0)
    return Number;
  auto Choice = G.choose(2);
  if (Choice == 1)
    return Number;
  return test_right_skewed_tree_left_tree(G, Depth - 1, Number + 1);
}

static unsigned long test_right_skewed_tree_helper(uniform::Generator &G,
                                                   int Depth, int Number) {
  if (Depth == 0)
    return Number;
  auto Choice = G.choose(2);
  if (Choice == 0)
    return test_right_skewed_tree_left_tree(G, Depth - 1, Number);
  return test_right_skewed_tree_helper(G, Depth - 1, Number + Depth);
}

static unsigned long test_right_skewed_tree(uniform::Generator &G) {
  const int TreeDepth = 6;

  return test_right_skewed_tree_helper(G, TreeDepth, 0);
}
/*
 * This tree offers a long dangly path with "bushes" (complete or
 * nearly-complete binary trees) hanging off each branch of the path, with the
 * other branch leading to many more leaves. The direction of the long branch
 * zigs and zags at each stage to mess with any attempt to find a consistent
 * ordering of the paths.
 *
 * A notable feature of this test is that trying to traverse it breadth first
 * (in any order) will go wrong because it will get caught in a bush rather than
 * finding where the bulk of the tree lies.
 */

static unsigned long test_path_with_thickets_bush(uniform::Generator &G,
                                                  unsigned long Size,
                                                  unsigned long Number) {
  assert(Size > 0);
  if (Size == 1)
    return Number;

  unsigned long LargeSubtreeSize = Size / 2;

  if (G.choose(2) == 0)
    return test_path_with_thickets_bush(G, LargeSubtreeSize, Number);
  else
    return test_path_with_thickets_bush(G, Size - LargeSubtreeSize,
                                        Number + LargeSubtreeSize);
}

static unsigned long test_path_with_thickets_helper(uniform::Generator &G,
                                                    unsigned long Size,
                                                    unsigned long Number,
                                                    unsigned long BushSize,
                                                    bool BushLeft) {
  assert(Size > 0);

  if (Size <= BushSize)
    return test_path_with_thickets_bush(G, Size, Number);

  if (BushLeft) {
    if (G.choose(2) == 0)
      return test_path_with_thickets_bush(G, BushSize, Number);
    else
      return test_path_with_thickets_helper(
          G, Size - BushSize, Number + BushSize, BushSize, !BushLeft);
  } else {
    if (G.choose(2) == 1)
      return test_path_with_thickets_bush(G, BushSize,
                                          Number + Size - BushSize);
    else
      return test_path_with_thickets_helper(G, Size - BushSize, Number,
                                            BushSize, !BushLeft);
  }
}

static unsigned long test_path_with_thickets(uniform::Generator &G) {
  const int Size = 50;
  const int BushSize = 8;

  return test_path_with_thickets_helper(G, Size, 0, BushSize, true);
}

//////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
static const bool Debug = true;
#else
static const bool Debug = false;
#endif

void run_test(std::string Name,
              unsigned long (*TestFunction)(uniform::Generator &)) {
  const int REPS = 1000 * 1000;
  std::vector<int> Results;
  uniform::Generator G;

  auto hline = std::string(40, '-');

  std::cout << std::endl;
  std::cout << hline << std::endl;
  std::cout << "Running tests for " << Name << std::endl;
  std::cout << hline << std::endl;

  for (int rep = 0; rep < REPS; ++rep) {
    if (!G.start())
      break;
    auto Res = TestFunction(G);
    if (Debug)
      std::cout << "Res = " << Res << "\n";
    if (Res >= Results.size())
      Results.resize(Res + 1);
    ++Results.at(Res);
    G.finish();
  }

  int total = 0;
  for (unsigned long i = 0; i < Results.size(); ++i) {
    std::cout << i << " : " << Results.at(i) << "\n";
    total += Results.at(i);
  }
  std::cout << "total = " << total << "\n";
  assert(total == REPS);

  std::cout << "Done.\n";
}

#define RUN_TEST(TEST) run_test(#TEST, test_##TEST)

int main() {
  RUN_TEST(maximally_unbalanced);
  RUN_TEST(full_tree);
  RUN_TEST(right_skewed_tree);
  RUN_TEST(path_with_thickets);

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
