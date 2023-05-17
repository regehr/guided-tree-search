#ifndef STANDARD_TREE_HELPERS_H_
#define STANDARD_TREE_HELPERS_H_

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
 * tree that we precompute totally randomly in advance, bounded only
 * by depth and maximum degree
 */

// TODO

/*
 * maximally unbalanced n-ary tree
 */
static uint64_t test_maximally_unbalanced_helper(tree_guide::Chooser &C,
                                                 int Depth, uint64_t Number,
                                                 uint64_t BranchFactor) {
  if (Depth == 0)
    return Number;
  auto Choice = C.choose(BranchFactor);
  if (Choice != (BranchFactor - 1))
    return Number + Choice;
  return test_maximally_unbalanced_helper(
      C, Depth - 1, Number + BranchFactor - 1, BranchFactor);
}

static uint64_t test_maximally_unbalanced(tree_guide::Chooser &C,
                                          uint64_t &NumLeaves) {
  const int TreeDepth = 5;
  const int BranchFactor = 17;
  NumLeaves = (BranchFactor - 1) * (TreeDepth - 1) + BranchFactor;

  return test_maximally_unbalanced_helper(C, TreeDepth, 0, BranchFactor);
}

/*
 * full tree
 */
static uint64_t test_full_tree_helper(tree_guide::Chooser &C, int Depth,
                                      uint64_t Number, int BranchFactor) {
  if (Depth == 0) {
    return Number;
  } else {
    return test_full_tree_helper(
        C, Depth - 1, (BranchFactor * Number) + C.choose(BranchFactor),
        BranchFactor);
  }
}

// x^y
static uint64_t ipow(uint64_t x, uint64_t y) {
  uint64_t Result = 1;
  for (uint64_t i = 0; i < y; ++i)
    Result *= x;
  return Result;
}

static uint64_t test_full_tree(tree_guide::Chooser &C, uint64_t &NumLeaves) {
  const int TreeDepth = 6;
  const int BranchFactor = 2;
  NumLeaves = ipow(BranchFactor, TreeDepth);

  return test_full_tree_helper(C, TreeDepth, 0, BranchFactor);
}

/*
 * Skewed tree: There is a uint64_t right-leaning path of depth Depth, and off
 * every point in that path there is a left leaning path that goes to the same
 * depth. The way to hit a leaf is either to go all the way down to the maximum
 * depth, or the go left once and then right at any point after that.
 *
 * A notable feature of this test is that the left branches are much smaller
 * than they look, and the right branches off the main path are quadratically
 * larger than the left branches, so should be picked much more often.
 */

static uint64_t test_right_skewed_tree_left_tree(tree_guide::Chooser &C,
                                                 int Depth, int Number) {

  if (Depth == 0)
    return Number;
  auto Choice = C.choose(2);
  if (Choice == 1)
    return Number;
  return test_right_skewed_tree_left_tree(C, Depth - 1, Number + 1);
}

static uint64_t test_right_skewed_tree_helper(tree_guide::Chooser &C, int Depth,
                                              int Number) {
  if (Depth == 0)
    return Number;
  auto Choice = C.choose(2);
  if (Choice == 0)
    return test_right_skewed_tree_left_tree(C, Depth - 1, Number);
  return test_right_skewed_tree_helper(C, Depth - 1, Number + Depth);
}

static uint64_t test_right_skewed_tree(tree_guide::Chooser &C,
                                       uint64_t &NumLeaves) {
  const int TreeDepth = 6;

  // A left tree of depth N has N + 1 leaves - one on the right, and N
  // for each left branch.
  // A right tree of depth 0 has 1 leaf. A right tree of depth N + 1 has
  // a left tree of depth N and a right tree of depth N underneath it,
  // so the size R_n is defined by R_0 = 1, and R_{n + 1} = R_n + L_n.
  // = R_n + n + 1.
  // So R_n = n + ... + 1 R_0 = n (n + 1) / 2 + 1

  NumLeaves = (6 * 7) / 2 + 1;

  return test_right_skewed_tree_helper(C, TreeDepth, 0);
}
/*
 * This tree offers a uint64_t dangly path with "bushes" (complete or
 * nearly-complete binary trees) hanging off each branch of the path, with the
 * other branch leading to many more leaves. The direction of the uint64_t
 * branch zigs and zags at each stage to mess with any attempt to find a
 * consistent ordering of the paths.
 *
 * A notable feature of this test is that trying to traverse it breadth first
 * (in any order) will go wrong because it will get caught in a bush rather than
 * finding where the bulk of the tree lies.
 */

static uint64_t test_path_with_thickets_bush(tree_guide::Chooser &C,
                                             uint64_t Size, uint64_t Number) {
  assert(Size > 0);
  if (Size == 1)
    return Number;

  uint64_t LargeSubtreeSize = Size / 2;

  if (C.choose(2) == 0)
    return test_path_with_thickets_bush(C, LargeSubtreeSize, Number);
  else
    return test_path_with_thickets_bush(C, Size - LargeSubtreeSize,
                                        Number + LargeSubtreeSize);
}

static uint64_t test_path_with_thickets_helper(tree_guide::Chooser &C,
                                               uint64_t Size, uint64_t Number,
                                               uint64_t BushSize,
                                               bool BushLeft) {
  assert(Size > 0);

  if (Size <= BushSize)
    return test_path_with_thickets_bush(C, Size, Number);

  if (BushLeft) {
    if (C.choose(2) == 0)
      return test_path_with_thickets_bush(C, BushSize, Number);
    else
      return test_path_with_thickets_helper(
          C, Size - BushSize, Number + BushSize, BushSize, !BushLeft);
  } else {
    if (C.choose(2) == 1)
      return test_path_with_thickets_bush(C, BushSize,
                                          Number + Size - BushSize);
    else
      return test_path_with_thickets_helper(C, Size - BushSize, Number,
                                            BushSize, !BushLeft);
  }
}

static uint64_t test_path_with_thickets(tree_guide::Chooser &C,
                                        uint64_t &NumLeaves) {
  const int Size = 50;
  const int BushSize = 8;
  NumLeaves = Size;

  return test_path_with_thickets_helper(C, Size, 0, BushSize, true);
}

/*
 * tree where degree increases by one every time we descend a level
 */

static uint64_t test_increasing_degree_tree_helper(tree_guide::Chooser &C,
                                                   int Depth, int Number,
                                                   int BranchFactor) {
  if (Depth == 0) {
    return Number;
  } else {
    return test_increasing_degree_tree_helper(
        C, Depth - 1, (BranchFactor * Number) + C.choose(BranchFactor),
        BranchFactor + 1);
  }
}

static uint64_t test_increasing_degree_tree(tree_guide::Chooser &C,
                                            uint64_t &NumLeaves) {
  const int TreeDepth = 6;
  NumLeaves = 1;
  for (int i = 1; i <= TreeDepth; ++i)
    NumLeaves *= i;

  return test_increasing_degree_tree_helper(C, TreeDepth, 0, 1);
}

/*
 * tree where degree decreases by one every time we descend a level
 */

static uint64_t test_decreasing_degree_tree_helper(tree_guide::Chooser &C,
                                                   int Depth, int Number,
                                                   int BranchFactor) {
  if (Depth == 0) {
    return Number;
  } else {
    return test_decreasing_degree_tree_helper(
        C, Depth - 1, (BranchFactor * Number) + C.choose(BranchFactor),
        BranchFactor - 1);
  }
}

static uint64_t test_decreasing_degree_tree(tree_guide::Chooser &C,
                                            uint64_t &NumLeaves) {
  const int TreeDepth = 6;
  NumLeaves = 1;
  for (int i = 1; i <= TreeDepth; ++i)
    NumLeaves *= i;

  return test_decreasing_degree_tree_helper(C, TreeDepth, 0, TreeDepth);
}

#endif
