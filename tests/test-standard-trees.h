#define TREE_TEST_CASE(TEST)                                                   \
  SECTION(#TEST) {                                                             \
    auto Tree = test_##TEST;                                                   \
    uint64_t NumLeaves = (uint64_t)-1;                                        \
    uint64_t RemainingLeaves = (uint64_t)-1;                                   \
    for (int rep = 0; rep < REPS; ++rep) {                                     \
      auto C = G.makeChooser();                                                \
      if (!C)                                                                  \
        break;                                                                 \
      auto Res = Tree(*C, NumLeaves);                                          \
      if (rep == 0) {                                                          \
        REQUIRE(NumLeaves > 0);                                                \
        RemainingLeaves = NumLeaves;                                           \
      }                                                                        \
      if (Res >= Results.size())                                               \
        Results.resize(Res + 1);                                               \
                                                                               \
      if (Results.at(Res) == 0) {                                              \
        ++Results.at(Res);                                                     \
        RemainingLeaves--;                                                     \
        if (RemainingLeaves == 0)                                              \
          break;                                                               \
      }                                                                        \
    }                                                                          \
    REQUIRE(NumLeaves > 0);                                                    \
    REQUIRE((size_t)NumLeaves == Results.size());                              \
    for (size_t i = 0; i < Results.size(); i++)                                \
      REQUIRE(Results.at(i) > 0);                                              \
  }

TEMPLATE_TEST_CASE("Can discover all leaves in standard trees",
                   "[test][template]", tree_guide::BFSGuide,
                   tree_guide::WeightedSamplerGuide) {
  TestType G;
  const int REPS = 10000;
  std::vector<int> Results;

  TREE_TEST_CASE(maximally_unbalanced);
  TREE_TEST_CASE(full_tree);
  TREE_TEST_CASE(right_skewed_tree);
  TREE_TEST_CASE(path_with_thickets);
  TREE_TEST_CASE(increasing_degree_tree);
  TREE_TEST_CASE(decreasing_degree_tree);
}
