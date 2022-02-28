TEMPLATE_TEST_CASE("Can discover all leaves in standard trees",
                   "[test][template]", tree_guide::BFSGuide,
                   tree_guide::WeightedSamplerGuide) {
  TestType G;
  const int REPS = 10000;
  std::vector<int> Results;

  auto Tree =
      GENERATE(test_maximally_unbalanced, test_full_tree,
               test_right_skewed_tree, test_path_with_thickets,
               test_increasing_degree_tree, test_decreasing_degree_tree);

  bool EarlyExit = false;
  long NumLeaves = -1;
  long RemainingLeaves = -1;
  for (int rep = 0; rep < REPS; ++rep) {
    auto C = G.makeChooser();
    if (!C) {
      EarlyExit = true;
      break;
    }
    auto Res = Tree(*C, NumLeaves);
    if (rep == 0) {
      REQUIRE(NumLeaves > 0);
      RemainingLeaves = NumLeaves;
    }
    if (Res >= Results.size())
      Results.resize(Res + 1);

    if (Results.at(Res) == 0) {
      ++Results.at(Res);
      RemainingLeaves--;
      if (RemainingLeaves == 0)
        break;
    }
  }

  REQUIRE(NumLeaves > 0);
  REQUIRE((size_t)NumLeaves == Results.size());
  for (size_t i = 0; i < Results.size(); i++)
    REQUIRE(Results.at(i) > 0);
}
