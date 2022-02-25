template <typename F> std::vector<double> frequencies(F TestFunction) {
  const int REPS = 2000;
  tree_guide::WeightedSamplerGuide G;

  std::vector<size_t> counts;
  for (int rep = 0; rep < REPS; ++rep) {
    auto C = G.makeChooser();
    auto i = TestFunction(*C);
    while (i >= counts.size())
      counts.push_back(0);
    counts[i] += 1;
  }
  std::vector<double> result;
  for (auto n : counts)
    result.push_back((double)n / (double)REPS);
  return result;
}

TEST_CASE("Basic frequency checks") {
  SECTION("Simple unbalanced tree") {
    std::vector<double> freq =
        frequencies([](tree_guide::Chooser &chooser) -> unsigned long {
          if (chooser.flip())
            return 0;
          if (chooser.flip())
            return 1;
          return 2;
        });
    REQUIRE(freq[0] >= 0.3);
    REQUIRE(freq[1] >= 0.3);
    REQUIRE(freq[2] >= 0.3);
  }

  SECTION("Uniform weighted choice") {
    std::vector<double> freq =
        frequencies([](tree_guide::Chooser &chooser) -> unsigned long {
          std::vector<double> weights = {1.0, 1.0, 1.0};
          return chooser.chooseWeighted(weights);
        });
    REQUIRE(freq[0] >= 0.3);
    REQUIRE(freq[1] >= 0.3);
    REQUIRE(freq[2] >= 0.3);
  }

  SECTION("Non-uniform weighted choice") {
    std::vector<double> freq =
        frequencies([](tree_guide::Chooser &chooser) -> unsigned long {
          std::vector<double> weights = {0.6, 0.2, 0.2};
          return chooser.chooseWeighted(weights);
        });
    // Probabilities should be be unadjusted
    REQUIRE(freq[0] >= 0.5);
    REQUIRE(freq[1] >= 0.1);
    REQUIRE(freq[2] >= 0.1);
  }

  SECTION("Non-uniform weighted choice with follow on") {
    std::vector<double> freq =
        frequencies([](tree_guide::Chooser &chooser) -> unsigned long {
          std::vector<double> weights = {0.6, 0.2, 0.2};
          auto result = chooser.chooseWeighted(weights);
          chooser.flip();
          chooser.flip();
          return result;
        });
    // Probabilities should be be unadjusted
    REQUIRE(freq[0] >= 0.5);
    REQUIRE(freq[1] >= 0.1);
    REQUIRE(freq[2] >= 0.1);
  }

  SECTION("Reweighted unbalanced tree") {
    std::vector<double> freq =
        frequencies([](tree_guide::Chooser &chooser) -> unsigned long {
          std::vector<double> frequencies = {2.0 / 3, 4.0 / 3};

          if (chooser.chooseWeighted(frequencies) == 1)
            return 0;
          if (chooser.flip())
            return 1;
          return 2;
        });

    // Probabilities should be reweighted to [0.5, 0.25, 0.25]
    REQUIRE(freq[0] >= 0.4);
    REQUIRE(freq[1] >= 0.2);
    REQUIRE(freq[2] >= 0.2);
  }
}
