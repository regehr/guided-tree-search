#ifndef TREE_GUIDE_H_
#define TREE_GUIDE_H_

#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>

namespace tree_guide {

static const bool Verbose = false;

////////////////////////////////////////////////////////////////////////////////

/*
 * abstract base classes for all of the guides and choosers
 */

class Chooser {
protected:
  Chooser() {}

public:
  virtual ~Chooser() {}
  // return a number in 0..n
  virtual uint64_t choose(uint64_t n) = 0;
  // shorthand for choose(2)
  virtual bool flip() = 0;
  // weighted choice
  virtual uint64_t chooseWeighted(const std::vector<double> &) = 0;
  virtual uint64_t chooseWeighted(const std::vector<uint64_t> &) = 0;
  /*
   * this call has a very specific contract: it does not cause the
   * decision tree to branch; it must only be used when the value that
   * is returned will not affect subsequent decisions made by the
   * generator. it might be used, for example, to generate a literal
   * constant in the output, or the name of an identifer
   */
  virtual uint64_t chooseUnimportant() = 0;
  virtual void beginScope() = 0;
  virtual void endScope() = 0;
};

class Guide {
public:
  Guide() {}
  Guide(uint64_t) {}
  virtual ~Guide() {}
  virtual std::unique_ptr<Chooser> makeChooser() = 0;
  virtual const std::string name() = 0;
};

////////////////////////////////////////////////////////////////////////////////

/*
 * DefaultGuide: it's just a wrapped PRNG. the point of this class is
 * to offer the naive alternative to the smarter generators, as a
 * baseline for comparison and so people can get used to the API
 * without anything interesting going on
 */

class DefaultGuide;

class DefaultChooser : public Chooser {
  DefaultGuide &G;

public:
  inline DefaultChooser(DefaultGuide &_G) : G(_G) {}
  inline ~DefaultChooser() {}
  inline uint64_t choose(uint64_t Choices) override;
  inline bool flip() override { return choose(2); }
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline void beginScope() override {}
  inline void endScope() override {}
};

class DefaultGuide : public Guide {
  friend DefaultChooser;
  std::unique_ptr<std::mt19937_64> Rand;

public:
  inline DefaultGuide(uint64_t Seed) {
    Rand = std::make_unique<std::mt19937_64>(Seed);
  }
  inline DefaultGuide() : DefaultGuide(std::random_device{}()) {}
  inline ~DefaultGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override {
    return std::make_unique<DefaultChooser>(*this);
  }
  inline const std::string name() override { return "default"; }
};

uint64_t DefaultChooser::choose(uint64_t Choices) {
  std::uniform_int_distribution<int> Dist(0, Choices - 1);
  return Dist(*G.Rand.get());
}

uint64_t DefaultChooser::chooseWeighted(const std::vector<double> &Probs) {
  std::discrete_distribution<uint64_t> Discrete(Probs.begin(), Probs.end());
  return Discrete(*G.Rand.get());
}

uint64_t DefaultChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  std::discrete_distribution<uint64_t> Discrete(Probs.begin(), Probs.end());
  return Discrete(*G.Rand.get());
}

inline uint64_t fullRange(std::mt19937_64 &G) {
  std::uniform_int_distribution<uint64_t> Dist(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  return Dist(G);
}

uint64_t DefaultChooser::chooseUnimportant() {
  return fullRange(*G.Rand.get());
}

////////////////////////////////////////////////////////////////////////////////

/*
 * BFSGuide: exhaustive breadth-first exploration of the decision
 * tree, reverting to random choices once beyond the BFS frontier
 */

// TODO this guide uses way too much memory -- since it never frees,
// it shouldn't be too difficult to replace its allocated cells with a
// large flat allocation

template <typename T> class PriQ {
  struct Elt {
    std::vector<T> Vec;
    uint64_t StartPos;
  };
  std::vector<Elt> Data;
  uint64_t Highest = (uint64_t)-1;
  const uint64_t MaxFree = 256;

public:
  /*
   * insert element at given level
   */
  void insert(T t, uint64_t Level) {
    if (Level >= (uint64_t)Data.size())
      Data.resize(Level + 1);
    Data.at(Level).Vec.push_back(t);
    if (Highest == (uint64_t)-1 || Level < Highest)
      Highest = Level;
  }

  /*
   * remove item from the given level
   */
  std::optional<T> remove(uint64_t Level) {
    if (Level >= (uint64_t)Data.size())
      return {};
    if (count(Level) < 1)
      return {};
    auto &Q = Data.at(Level);
    auto t = Q.Vec.at(Q.StartPos);
    Q.StartPos++;
    if (Q.StartPos > MaxFree) {
      Q.Vec.erase(Q.Vec.begin(), Q.Vec.begin() + Q.StartPos);
      Q.StartPos = 0;
    }
    if (Level == Highest && count(Level) == 0) {
      Highest = (uint64_t)-1;
      for (uint64_t L = Level + 1; L < (uint64_t)Data.size(); ++L) {
        if (count(L) > 0) {
          Highest = L;
          break;
        }
      }
    }
    return t;
  }

  /*
   * remove highest-priority item at any level
   */
  std::pair<std::optional<T>, uint64_t> removeHead() {
    auto L = firstNonemptyLevel();
    if (L == (uint64_t)-1)
      return {{}, (uint64_t)-1};
    else
      return {remove(L), L};
  }

  /*
   * return number of items at this level
   */
  uint64_t count(uint64_t Level) {
    if (Level >= Data.size())
      return 0;
    return Data.at(Level).Vec.size() - Data.at(Level).StartPos;
  }

  /*
   * return the smallest level that is nonempty, or else -1 if all
   * levels are empty
   */
  uint64_t firstNonemptyLevel() { return Highest; }
};

class BFSChooser;

class BFSGuide : public Guide {
  friend BFSChooser;
  struct Node {
    Node *Parent;
    std::vector<std::unique_ptr<BFSGuide::Node>> Children;
  };

  uint64_t TotalNodes = 0;
  std::unique_ptr<BFSGuide::Node> Root;
  PriQ<Node *> PendingPaths;
  uint64_t MaxSavedLevel = (uint64_t)-1;
  bool Choosing = false, Started = false;
  // TODO move this into the chooser?
  std::unique_ptr<std::mt19937_64> Rand;

public:
  inline BFSGuide(uint64_t Seed);
  inline BFSGuide() : BFSGuide(std::random_device{}()) {}
  inline ~BFSGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override;
  inline const std::string name() override { return "BFS"; }
};

class BFSChooser : public Chooser {
  friend BFSGuide;
  BFSGuide &G;
  BFSGuide::Node *Current;
  uint64_t LastChoice = 0, Level = 0;
  // this vector is in reverse order so we can pop stuff efficiently
  std::vector<uint64_t> SavedChoices;
  inline uint64_t chooseInternal(uint64_t, std::function<uint64_t()>);

public:
  inline BFSChooser(BFSGuide &_G) : G(_G) { Current = &*G.Root; }
  inline ~BFSChooser();
  inline uint64_t choose(uint64_t Choices) override;
  inline bool flip() override;
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline void beginScope() override {}
  inline void endScope() override {}
};

BFSGuide::BFSGuide(uint64_t Seed) {
  Root = std::make_unique<BFSGuide::Node>();
  Root->Children.resize(1);
  Rand = std::make_unique<std::mt19937_64>(Seed);
}

std::unique_ptr<Chooser> BFSGuide::makeChooser() {
  if (Verbose)
    std::cout << "*** START *** (total nodes = " << TotalNodes << ")\n";
  assert(!Choosing);
  /*
   * case 1: this is the first traversal; we've not yet seen any of
   * the decision tree, so do a purely random traversal to bootstrap
   * things
   */
  if (!Started) {
    if (Verbose)
      std::cout << "  First traversal\n";
    Started = true;
    Choosing = true;
    return std::make_unique<BFSChooser>(*this);
  }
  /*
   * case 2: the priority queue has unexplored decisions for us to
   * traverse, this is where we spent most of our time of course
   */
  auto [OptionalNode, SavedLevel] = PendingPaths.removeHead();
  if (OptionalNode.has_value()) {
    assert((MaxSavedLevel == (uint64_t)-1) || (SavedLevel >= MaxSavedLevel));
    if (Verbose && SavedLevel > MaxSavedLevel)
      std::cout << "fully explored up to " << SavedLevel << "\n";
    MaxSavedLevel = SavedLevel;
    auto C = std::make_unique<BFSChooser>(*this);

    auto N = OptionalNode.value();
    BFSGuide::Node *N2 = nullptr;
    // this loop walks up to the root, saving the decisions that we
    // have to make to get back down here
    do {
      uint64_t Next = (uint64_t)-1;
      uint64_t S = N->Children.size();
      if (N2) {
        // we're above the target node, so just get to the target
        for (uint64_t i = 0; i < S; ++i) {
          if (N->Children.at(i).get() == N2) {
            Next = i;
            break;
          }
        }
        if (Verbose)
          std::cout << "  appending " << Next
                    << " to saved choice above target node\n";
      } else {
        // we're at the target node, so find an untaken branch
        // TODO: this is deterministic, it would be better to pick a random one
        uint64_t NumUntaken = 0;
        for (uint64_t i = 0; i < S; ++i) {
          if (Verbose)
            std::cout << "    child " << i << " = " << N->Children.at(i).get()
                      << "\n";
          if (N->Children.at(i).get() == nullptr) {
            NumUntaken++;
            Next = i;
          }
        }
        if (Verbose)
          std::cout << "  appending " << Next
                    << " to saved choice at target node\n";
        // this node should not have been there if there wasn't a branch
        // left to explore
        assert(NumUntaken > 0);
        // if there's at least one remaining unexplored branch, put
        // this node back at the end of its priority queue
        if (NumUntaken > 1) {
          if (Verbose)
            std::cout << "  Re-inserting node\n";
          PendingPaths.insert(N, SavedLevel);
        }
      }
      assert(Next != (uint64_t)-1);
      C->SavedChoices.push_back(Next);
      N2 = N;
      N = N->Parent;
    } while (N != Root.get());
    Choosing = true;
    return C;
  }
  /*
   * case 3: the priority queue has run out of things for us to
   * explore; we're done. this is not going to happen in practice for
   * realistic applications. however, in the future we might wish to
   * implement uniform sampling of the leaves; now that we have the
   * entire decision tree this is not difficult. sampling a leaf more
   * than once only makes sense if we allow random decisions that
   * don't cause branching in the tree, generators could use this to
   * generate things like wide literal constants
   */
  if (Verbose)
    std::cout << "  Tree has been completely explored!\n";
  return nullptr;
}

BFSChooser::~BFSChooser() {
  assert(SavedChoices.empty());
  // TODO -- at scale this allocation will double our RAM usage, so
  // eventually do this a different way
  if (!Current->Children.at(LastChoice).get()) {
    Current->Children.at(LastChoice) = std::make_unique<BFSGuide::Node>();
    G.TotalNodes++;
  }
  G.Choosing = false;
}

uint64_t BFSChooser::chooseInternal(const uint64_t Choices,
                                    std::function<uint64_t()> randomChoice) {
  assert(G.Choosing);
  if (Verbose) {
    std::cout << "choose(" << Choices << ")\n";
    std::cout << "  Current = " << Current << ", LastChoice = " << LastChoice
              << "\n";
  }

  uint64_t Choice;
  auto N = Current->Children.at(LastChoice).get();
  if (Verbose)
    std::cout << "Node pointer = " << N << "\n";
  if (N) {
    /*
     * we've arrived at a tree node that has already been visited
     */
    if (Choices != N->Children.size()) {
      // TODO it's unfriendly to exit here, but this is a critical API
      // violation. alternatively, of course we could throw an
      // exception
      std::cout << "FATAL ERROR: Reached same node again, but different "
                   "number of choices this time\n\n";
      exit(-1);
    }
    uint64_t NumSavedChoices = SavedChoices.size();
    if (Verbose)
      std::cout << "  There are " << NumSavedChoices << " saved choices\n";
    assert(NumSavedChoices > 0);
    Choice = SavedChoices.at(NumSavedChoices - 1);
    if (Verbose)
      std::cout << "  We'll be taking option " << Choice << "\n";
    SavedChoices.pop_back();
  } else {
    /*
     * we're off the beaten path, add this decision node to the tree
     * and make a random choice
     */
    assert(SavedChoices.size() == 0);
    N = new BFSGuide::Node;
    G.TotalNodes++;
    N->Parent = Current;
    N->Children.resize(Choices);
    auto UN = std::unique_ptr<BFSGuide::Node>(N);
    Current->Children.at(LastChoice) = std::move(UN);
    Choice = randomChoice();
    /*
     * if there are other options we'll need to get back to them later
     */
    if (Choices > 1) {
      if (Verbose)
        std::cout << "  Inserting node " << N << " at level " << Level
                  << " with degree " << Choices << "\n";
      G.PendingPaths.insert(N, Level);
    }
  }
  Current = N;
  LastChoice = Choice;
  Level++;
  if (Verbose)
    std::cout << "  returning " << Choice << "\n";
  return Choice;
}

uint64_t BFSChooser::choose(uint64_t Choices) {
  return chooseInternal(Choices, [&]() -> uint64_t {
    std::uniform_int_distribution<int> Dist(0, Choices - 1);
    return Dist(*G.Rand);
  });
}

bool BFSChooser::flip() { return choose(2); }

uint64_t BFSChooser::chooseWeighted(const std::vector<double> &Probs) {
  return chooseInternal(Probs.size(), [&]() -> uint64_t {
    std::discrete_distribution<uint64_t> Discrete(Probs.begin(), Probs.end());
    return Discrete(*G.Rand.get());
  });
}

uint64_t BFSChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  return chooseInternal(Probs.size(), [&]() -> uint64_t {
    std::discrete_distribution<uint64_t> Discrete(Probs.begin(), Probs.end());
    return Discrete(*G.Rand.get());
  });
}

uint64_t BFSChooser::chooseUnimportant() { return fullRange(*G.Rand.get()); }

////////////////////////////////////////////////////////////////////////////////

/*
 * EnumeratingGuide: exhaustive depth-first exploration of the decision
 * tree. This guide should typically be used when the search space is
 * small enough to be fully explored.
 */

class EnumeratingChooser;

class EnumeratingGuide : public Guide {
  friend EnumeratingChooser;
  /// The last choices made in the decision tree.
  std::vector<uint64_t> LastPath;
  /// The number of choices at each level for the last path.
  std::vector<uint64_t> NumChoices;
  /// Has any chooser been created yet?
  bool Started = false;
  /// Is there an existing chooser that is currently making choices?
  bool Choosing = false;
  uint64_t TotalNodes = 0;

public:
  inline EnumeratingGuide() : LastPath(), NumChoices() {};
  inline ~EnumeratingGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override;
  inline const std::string name() override { return "Enumerating"; }
};

class EnumeratingChooser : public Chooser {
  friend EnumeratingGuide;
  EnumeratingGuide &G;

  uint64_t Level = 0;
  /// The choices that are made at each level.
  /// It is first initialized with the choices that this chooser has to make at
  /// the first levels, and then it is updated with the new choices that are
  /// made at each level.
  std::vector<uint64_t> CurrentPath;
  /// The number of choices at each level.
  /// It is first initialized with the known number of choices that this
  /// chooser has for the first levels, and then it is updated with the number
  /// of choices that are available at each new level.
  std::vector<uint64_t> NumChoices;
  inline uint64_t chooseInternal(uint64_t NumChoices);

public:
  inline EnumeratingChooser(EnumeratingGuide &_G,
                            std::vector<uint64_t> CurrentPath,
                            std::vector<uint64_t> NumChoices)
      : G(_G), Level(0), CurrentPath(std::move(CurrentPath)),
        NumChoices(std::move(NumChoices)) {}
  inline ~EnumeratingChooser();
  inline uint64_t choose(uint64_t Choices) override;
  inline bool flip() override { return choose(2); }
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline void beginScope() override {}
  inline void endScope() override {}
};

std::unique_ptr<Chooser> EnumeratingGuide::makeChooser() {
  if (Verbose)
    std::cout << "*** START *** (total nodes = " << TotalNodes << ")\n";
  assert(!Choosing && "A chooser is already constructed");

  auto NextPath = LastPath;
  for (int i = (int)LastPath.size() - 1; i >= 0; i--) {
    if (NextPath[i] == NumChoices[i] - 1) {
      NextPath.pop_back();
      continue;
    }
    NextPath[i]++;
    break;
  }
  std::vector<uint64_t> NextNumChoices(NumChoices.begin(),
                                       NumChoices.begin() + NextPath.size());

  if (Started && NextPath.empty()) {
    if (Verbose)
      std::cout << "  Tree has been completely explored!\n";
    return nullptr;
  }

  if (!Started) {
    if (Verbose)
      std::cout << "  First traversal\n";
    Started = true;
  }

  Choosing = true;
  auto C =
      std::make_unique<EnumeratingChooser>(*this, NextPath, NextNumChoices);
  return C;
}

EnumeratingChooser::~EnumeratingChooser() {
  G.LastPath = CurrentPath;
  G.NumChoices = NumChoices;
  G.Choosing = false;
  G.TotalNodes++;
}

uint64_t EnumeratingChooser::chooseInternal(const uint64_t Choices) {
  assert(G.Choosing);
  if (Level < NumChoices.size()) {
    if (Choices != NumChoices[Level]) {
      std::cout << "FATAL ERROR: Reached same node again, but different "
                   "number of choices this time\n\n";
      exit(-1);
    }
    uint64_t Choice = CurrentPath[Level];
    if (Verbose)
      std::cout << "  Returning choice " << Choice << " at level " << Level
                << "\n";
    Level++;
    return CurrentPath[Level - 1];
  }

  NumChoices.push_back(Choices);
  CurrentPath.push_back(0);
  if (Verbose)
    std::cout << "  Adding new level " << Level << " with " << Choices
              << " choices\n";
  Level++;
  return 0;
}

uint64_t EnumeratingChooser::choose(uint64_t Choices) {
  return chooseInternal(Choices);
}

uint64_t EnumeratingChooser::chooseWeighted(const std::vector<double> &Probs) {
  return chooseInternal(Probs.size());
}

uint64_t
EnumeratingChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  return chooseInternal(Probs.size());
}

uint64_t EnumeratingChooser::chooseUnimportant() {
  std::cerr
      << "  EnumeratingChooser::chooseUnimportant() is not implemented.\n";
  exit(1);
}

////////////////////////////////////////////////////////////////////////////////

/*
 * WeightedSamplerChooser: tries to explore subtrees of the decision
 * tree in an intelligent fashion using techniques resembling
 * cardinality estimation
 */

class WeightedSamplerChooser;

class WeightedSamplerGuide : public Guide {
  friend WeightedSamplerChooser;

  struct Node {
    bool visited = false;
    size_t BranchFactor;
    std::vector<double> Weights;
    std::unordered_map<uint64_t, std::unique_ptr<Node>> Children;
    double SizeEstimate;

    inline Node() {}

    inline void visit(size_t n, const std::vector<double> &weights) {
      assert(weights.size() == 0 || weights.size() == n);
      if (this->visited) {
        assert(n == this->BranchFactor);
        return;
      } else if (n == 0) {
        this->BranchFactor = n;
        this->visited = true;
        this->SizeEstimate = 1.0;
      } else {
        this->BranchFactor = n;
        this->visited = true;
        this->SizeEstimate = n;
        if (weights.size() > 0) {
          double total = 0.0;
          for (const auto x : weights)
            total += x;
          for (const auto x : weights)
            this->Weights.push_back(x / total * n);
        }
      }
    }

    inline double weight(size_t i) {
      assert(this->visited);
      if (this->Weights.size() > 0) {
        return this->Weights[i];
      } else {
        return 1.0;
      }
    }

    inline void visit(size_t n) {
      std::vector<double> empty;
      this->visit(n, empty);
    }

    inline void debug(size_t indent) {
      assert(this->visited);
      if (this->Children.size() == 0) {
        std::cout << "Leaf node" << std::endl;
        return;
      }

      std::string indent_string(indent, ' ');
      std::cout << "Node (size estimate " << this->SizeEstimate << ");"
                << std::endl;
    }
  };

  std::unique_ptr<Node> Root;
  std::unique_ptr<std::mt19937_64> Rand;

public:
  inline WeightedSamplerGuide(uint64_t Seed) {
    this->Root = std::make_unique<Node>();
    this->Rand = std::make_unique<std::mt19937_64>(Seed);
  }
  inline WeightedSamplerGuide() : WeightedSamplerGuide(0) {}
  inline ~WeightedSamplerGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override;
  inline void debugTree() { this->Root->debug(0); }
  inline const std::string name() override { return "weighted sample"; }
};

class WeightedSamplerChooser : public Chooser {
  WeightedSamplerGuide &G;
  std::vector<WeightedSamplerGuide::Node *> Trail;

public:
  inline WeightedSamplerChooser(WeightedSamplerGuide &_G) : G(_G) {
    this->Trail.push_back(this->G.Root.get());
  }
  inline ~WeightedSamplerChooser() override {
    this->Trail.back()->visit(0);
    this->Trail.pop_back();
    while (this->Trail.size() > 0) {
      WeightedSamplerGuide::Node *last = this->Trail.back();
      double occupied = 0.0;
      double total = 0.0;
      for (auto &t : last->Children) {
        auto i = t.first;
        auto &child = t.second;
        if (child == nullptr)
          continue;
        auto weight = last->weight(i);
        total += child->SizeEstimate * weight;
        occupied += weight;
      }

      last->SizeEstimate = last->Children.size() * total / occupied;

      this->Trail.pop_back();
    }
  };

  inline uint64_t choose(uint64_t Choices, const std::vector<double> &Weights) {
    WeightedSamplerGuide::Node *current = this->Trail.back();
    current->visit(Choices, Weights);

    size_t result;
    WeightedSamplerGuide::Node *next_node;
    std::uniform_real_distribution<double> unif(0.0, 1.0);

    // When we visit a node we have to choose between whether to visit
    // a child we've already seen or not (unless we've already seen every
    // child or we've seen no children, in which case there is no choice).
    // We consider visiting a new child to be explore, and a previous child
    // to be exploit.
    //
    // The advantage of visiting previously visited children is we've got
    // a much firmer idea of what their structure is like, so we can choose
    // between them more sensibly. The advantage of visiting new children
    // is it allows us to explore previously unvisited areas of the search
    // space.
    //
    // We don't have any particularly good way of making this decision. The
    // current heuristic does its best to balance explore and exploit while
    // still trying to be useful for a large branch factor. As a result we
    // rapidly at first and then once we have a decent number of nodes to
    // compare, we switch to a more leisurely strategy where we prefer to
    // exploit existing nodes but explore occasionally.
    bool explore =
        (current->Children.size() < current->BranchFactor &&
         (current->Children.size() <= 5 || unif(*G.Rand.get()) <= 0.1));

    if (explore) {
      if (current->Weights.size() > 0) {
        std::discrete_distribution<uint64_t> Dist(current->Weights.begin(),
                                                  current->Weights.end());
        while (true) {
          result = Dist(*G.Rand.get());
          if (current->Children[result] == nullptr)
            break;
        }
      } else {
        std::uniform_int_distribution<uint64_t> Dist(0,
                                                     current->BranchFactor - 1);
        while (true) {
          result = Dist(*G.Rand.get());
          if (current->Children[result] == nullptr)
            break;
        }
      }

      next_node = (current->Children[result] =
                       std::make_unique<WeightedSamplerGuide::Node>())
                      .get();

    } else {
      std::vector<uint64_t> results;
      std::vector<double> weights;

      for (auto &t : current->Children) {
        auto value = t.first;
        auto &child = t.second;
        if (child == nullptr)
          continue;
        results.push_back(value);
        weights.push_back(current->weight(value) * child->SizeEstimate);
      }

      std::discrete_distribution<size_t> Dist(weights.begin(), weights.end());

      auto i = Dist(*G.Rand.get());

      result = results[i];

      next_node = current->Children[result].get();
    }

    assert(next_node != nullptr);

    this->Trail.push_back(next_node);
    return result;
  };

  inline uint64_t choose(uint64_t Choices) override {
    std::vector<double> empty;
    return this->choose(Choices, empty);
  }

  inline bool flip() override { return choose(2); }
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline void beginScope() override {}
  inline void endScope() override {}
};

std::unique_ptr<Chooser> WeightedSamplerGuide::makeChooser() {
  return std::make_unique<WeightedSamplerChooser>(*this);
}

uint64_t
WeightedSamplerChooser::chooseWeighted(const std::vector<double> &Probs) {
  return this->choose(Probs.size(), Probs);
}

uint64_t
WeightedSamplerChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  uint64_t Total = 0;
  for (auto I : Probs)
    Total += I;
  std::vector<double> V;
  for (auto I : Probs)
    V.push_back((double)I / Total);
  return this->choose(Probs.size(), V);
}

uint64_t WeightedSamplerChooser::chooseUnimportant() {
  return fullRange(*this->G.Rand);
}

////////////////////////////////////////////////////////////////////////////////

/*
 * SaverGuide: wraps another guide in order to remember choices that
 * it made; use the chooser's getChoices() or formatChoices() methods
 * to retreive them
 */

static const std::string StartMarker{"BEGIN FORMATTED CHOICES"};
static const std::string EndMarker{"END FORMATTED CHOICES"};

enum class RecKind { START = 777, END, NUM, NONE };

struct rec {
  RecKind k;
  uint64_t v;
};

class SaverChooser;

class SaverGuide : public Guide {
  friend SaverChooser;
  Guide *SubG;
  std::string Prefix;
  const size_t MAX_LINE_LENGTH = 70;

public:
  inline SaverGuide(uint64_t Seed) = delete;
  inline SaverGuide() = delete;
  inline SaverGuide(Guide *_SubG, const std::string &_Prefix)
      : SubG(_SubG), Prefix(_Prefix) {}
  inline ~SaverGuide() {}
  inline const std::string name() override {
    return SubG->name() + " (wrapped by Saver)";
  }
  inline std::unique_ptr<Chooser> makeChooser() override;
};

class SaverChooser : public Chooser {
  SaverGuide &G;
  std::unique_ptr<Chooser> C;
  std::vector<rec> Saved;

public:
  inline SaverChooser(SaverGuide &_G) : G(_G) { C = G.SubG->makeChooser(); }
  inline ~SaverChooser() {}
  inline uint64_t choose(uint64_t Choices) override;
  inline bool flip() override { return choose(2); }
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline const std::string formatChoices();
  inline std::vector<rec> &getChoices() { return Saved; }
  inline void beginScope() override;
  inline void endScope() override;
};

std::unique_ptr<Chooser> SaverGuide::makeChooser() {
  return std::make_unique<SaverChooser>(*this);
}

uint64_t SaverChooser::choose(uint64_t Choices) {
  auto X = C->choose(Choices);
  rec r{tree_guide::RecKind::NUM, X};
  Saved.push_back(r);
  return X;
}

uint64_t SaverChooser::chooseWeighted(const std::vector<double> &Probs) {
  auto X = C->chooseWeighted(Probs);
  rec r{tree_guide::RecKind::NUM, X};
  Saved.push_back(r);
  return X;
}

uint64_t SaverChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  auto X = C->chooseWeighted(Probs);
  rec r{tree_guide::RecKind::NUM, X};
  Saved.push_back(r);
  return X;
}

uint64_t SaverChooser::chooseUnimportant() {
  auto X = C->chooseUnimportant();
  rec r{tree_guide::RecKind::NUM, X};
  Saved.push_back(r);
  return X;
}

void SaverChooser::beginScope() {
  rec r{tree_guide::RecKind::START, 0};
  Saved.push_back(r);
  C->beginScope();
}

void SaverChooser::endScope() {
  rec r{tree_guide::RecKind::END, 0};
  Saved.push_back(r);
  C->endScope();
}

const std::string SaverChooser::formatChoices() {
  std::string s;
  s += G.Prefix + StartMarker + "\n";
  std::vector<rec>::size_type pos = 0;
  std::string line = G.Prefix;
  while (pos < Saved.size()) {
    std::string item;
    switch (Saved.at(pos).k) {
    case tree_guide::RecKind::START:
      item = "{";
      break;
    case tree_guide::RecKind::END:
      item = "}";
      break;
    case tree_guide::RecKind::NUM:
      item = std::to_string(Saved.at(pos).v);
      break;
    default:
      assert(false);
    }
    item += ",";
    if (line.length() + item.length() >= G.MAX_LINE_LENGTH) {
      s += line + "\n";
      line = G.Prefix + item;
    } else {
      line += item;
    }
    ++pos;
  }
  s += line + "\n";
  s += G.Prefix + EndMarker + "\n";
  return s;
}

////////////////////////////////////////////////////////////////////////////////

/*
 * FileGuide: loads a file of choices; every chooser that it returns
 * does exactly the same thing: makes the choices specified in the
 * file. this guide tries to reject syntactically invalid saved
 * choices, but also it tries to accept and deal with running out of
 * choices (it starts returning arbitrary values) and also
 * out-of-range choices (it reduces them to be in range)
 */

enum class Sync { NONE = 888, RESYNC, BALANCE };

class FileChooser;

class FileGuide : public Guide {
  friend FileChooser;
  std::vector<rec> Choices;
  std::unique_ptr<std::mt19937_64> Rand;
  Sync S = Sync::BALANCE;

public:
  inline FileGuide(uint64_t Seed) {
    Rand = std::make_unique<std::mt19937_64>(Seed);
  }
  inline FileGuide() : FileGuide(std::random_device{}()) {}
  inline ~FileGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override;
  inline void setSync(Sync _S) { S = _S; }
  inline const std::string name() override { return "file"; }
  inline bool parseChoices(std::istream &file, const std::string &Prefix);
  inline bool parseChoices(std::string &fileName, const std::string &Prefix);
  inline std::vector<rec> &getChoices() { return Choices; }
  inline void replaceChoices(const std::vector<rec> &C);
};

class FileChooser : public Chooser {
  FileGuide &G;
  std::vector<rec>::size_type Pos = 0;
  inline uint64_t nextVal();
  long FileDepth = 0, GeneratorDepth = 0;

public:
  inline FileChooser(FileGuide &_G) : G(_G) {}
  inline ~FileChooser();
  inline uint64_t choose(uint64_t Choices) override;
  inline bool flip() override { return choose(2); }
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline void beginScope() override { ++GeneratorDepth; }
  inline void endScope() override {
    --GeneratorDepth;
    if (G.S == Sync::BALANCE && GeneratorDepth < 0) {
      std::cerr
          << "FATAL ERROR: Negative nesting depth from generator side\n\n";
      exit(-1);
    }
  }
};

std::unique_ptr<Chooser> FileGuide::makeChooser() {
  return std::make_unique<FileChooser>(*this);
}

void FileGuide::replaceChoices(const std::vector<rec> &C) {
  Choices.clear();
  for (auto x : C)
    Choices.push_back(x);
}

bool FileGuide::parseChoices(std::istream &file, const std::string &Prefix) {
  std::string line;
  bool inData = false;
  auto PrefixLen = Prefix.size();
  while (std::getline(file, line)) {
    if (inData) {
      if (line == (Prefix + EndMarker)) {
        break;
      } else {
        if (line.compare(0, PrefixLen, Prefix) != 0) {
          std::cerr << "FATAL ERROR: Expected every line of choices to start "
                       "with '"
                    << Prefix << "'\n\n";
          return false;
        }
        uint64_t val = 0;
        RecKind k = tree_guide::RecKind::NONE;
        for (std::string::size_type pos = PrefixLen; pos < line.length();
             ++pos) {
          auto c = line[pos];
          if (c == ',') {
            rec r;
            switch (k) {
            case tree_guide::RecKind::NUM:
              r.v = val;
              val = 0;
              break;
            case tree_guide::RecKind::START:
              break;
            case tree_guide::RecKind::END:
              break;
            default:
              assert(false);
            }
            r.k = k;
            Choices.push_back(r);
            k = tree_guide::RecKind::NONE;
          } else if (c >= '0' && c <= '9') {
            // TODO check for integer overflow here, that could happen
            // if a choice sequence file got corrupted
            val *= 10;
            val += c - '0';
            k = tree_guide::RecKind::NUM;
          } else if (c == '{') {
            k = tree_guide::RecKind::START;
          } else if (c == '}') {
            k = tree_guide::RecKind::END;
          } else {
            std::cerr << "FATAL ERROR: Illegal character '" << c
                      << "' in choice string\n\n";
            std::cerr << "line: '" << line << "'\n";
            return false;
          }
        }
      }
    } else {
      if (line.find(Prefix + StartMarker) != std::string::npos)
        inData = true;
    }
  }
  if (Choices.size() == 0) {
    std::cerr << "FATAL ERROR: No choices could be parsed\n\n";
    return false;
  }
  return true;
}

bool FileGuide::parseChoices(std::string &FileName, const std::string &Prefix) {
  std::ifstream file(FileName);
  if (!file.is_open()) {
    std::cerr << "FATAL ERROR: Cannot open choice file '" << FileName
              << "'\n\n";
    return false;
  }
  auto res = parseChoices(file, Prefix);
  file.close();
  return res;
}

/*
 * each of the following functions should deal gracefully with both of
 * these conditions:
 *
 * - the choice from the file is out-of-bounds with respect to the
 *   requested range of values
 *
 * - we've run out of values from the file to return; in this case we
 *   additionally want the returned value to be deterministic and also
 *   not the same value every time (since this could cause a rejection
 *   sampling loop to run forever
 */

FileChooser::~FileChooser() {
  if (G.S == Sync::BALANCE) {
    if (GeneratorDepth != 0) {
      std::cerr << "FATAL ERROR: Unbalanced scopes from generator with depth "
                << GeneratorDepth << "\n\n";
      exit(-1);
    }
    // often there's (at least) an end scope still sitting there, we
    // need to process it
    while (Pos < G.Choices.size())
      nextVal();
    if (FileDepth != 0) {
      std::cerr << "FATAL ERROR: Unbalanced scopes from file with depth "
                << FileDepth << "\n\n";
      exit(-1);
    }
  }
}

uint64_t FileChooser::nextVal() {
again:

  // if we've exhausted the choice sequence from disk, we have no
  // choice besides returning randomness
  if (Pos >= G.Choices.size()) {
    if (Verbose)
      std::cerr << "Choice sequence exhausted, returning randomness\n";
    return fullRange(*G.Rand.get());
  }

  auto r = G.Choices.at(Pos);

  // next we give the file guide a chance to catch up with the scoping
  // level of the generator

  if (r.k == tree_guide::RecKind::START) {
    ++FileDepth;
    ++Pos;
    if (Verbose)
      std::cerr << "START: FileDepth is now " << FileDepth << "\n";
    goto again;
  }

  if (r.k == tree_guide::RecKind::END) {
    --FileDepth;
    if (Verbose)
      std::cerr << "END: FileDepth is now " << FileDepth << "\n";
    if (FileDepth < 0) {
      std::cerr << "FATAL ERROR: Negative nesting depth from file side\n\n";
      exit(-1);
    }
    ++Pos;
    goto again;
  }

  assert(r.k == tree_guide::RecKind::NUM);

  // now that we have a number to return, there are three choices,
  // where our overall goal us to force the two scope depths to line
  // up

  // already lined up -- no problem
  if (G.S != Sync::RESYNC || FileDepth == GeneratorDepth) {
    ++Pos;
    if (Verbose)
      std::cerr << "Returning number: " << r.v << "\n";
    return r.v;
  }

  // we want to avoid returning choices from the file
  if (FileDepth < GeneratorDepth) {
    auto v = fullRange(*G.Rand.get());
    if (Verbose)
      std::cerr << "Avoiding saved choice and returning random: " << v << "\n";
    return v;
  }

  // we want to discard choices from the file
  if (FileDepth > GeneratorDepth) {
    if (Verbose)
      std::cerr << "Discarding saved choice\n";
    ++Pos;
    goto again;
  }

  assert(false);
}

uint64_t FileChooser::choose(uint64_t Choices) { return nextVal() % Choices; }

uint64_t FileChooser::chooseWeighted(const std::vector<double> &Probs) {
  return nextVal() % Probs.size();
}

uint64_t FileChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  return nextVal() % Probs.size();
}

uint64_t FileChooser::chooseUnimportant() { return nextVal(); }

////////////////////////////////////////////////////////////////////////////////

/*
 * RRGuide: creates choosers from multiple guides in round-robin
 * fashion
 */

class RRChooser;

class RRGuide : public Guide {
  friend RRChooser;
  const std::vector<Guide *> Gs;
  size_t Current = 0;

public:
  inline RRGuide(uint64_t Seed) = delete;
  inline RRGuide() = delete;
  inline RRGuide(const std::vector<Guide *> &_Gs) : Gs(_Gs) {}
  inline ~RRGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override;
  inline const std::string name() override { return "round-robin"; }
};

class RRChooser : public Chooser {
  RRGuide &G;
  std::unique_ptr<Chooser> C;

public:
  inline RRChooser(RRGuide &_G) : G(_G) {
    // this is a bit awkward since some of the guides might start
    // failing to create choosers
    bool Reset = false;
    do {
      C = G.Gs.at(G.Current)->makeChooser();
      assert(C);
      ++G.Current;
      if (G.Current == G.Gs.size()) {
        G.Current = 0;
        Reset = true;
      }
    } while (C == nullptr && !Reset);
  }
  inline ~RRChooser() {}
  inline uint64_t choose(uint64_t Choices) override;
  inline bool flip() override { return choose(2); }
  inline uint64_t chooseWeighted(const std::vector<double> &) override;
  inline uint64_t chooseWeighted(const std::vector<uint64_t> &) override;
  inline uint64_t chooseUnimportant() override;
  inline bool hasSubChooser() { return C != nullptr; }
  inline void beginScope() override { C->beginScope(); }
  inline void endScope() override { C->endScope(); }
};

uint64_t RRChooser::choose(uint64_t Choices) { return C->choose(Choices); }

uint64_t RRChooser::chooseWeighted(const std::vector<double> &Probs) {
  return C->chooseWeighted(Probs);
}

uint64_t RRChooser::chooseWeighted(const std::vector<uint64_t> &Probs) {
  return C->chooseWeighted(Probs);
}

uint64_t RRChooser::chooseUnimportant() { return C->chooseUnimportant(); }

std::unique_ptr<Chooser> RRGuide::makeChooser() {
  auto C = std::make_unique<RRChooser>(*this);
  if (C->hasSubChooser())
    return C;
  else
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

/*
 * remote guide: ephemeral in-process guide that talks to a different
 * guide living in a server process; use this for generators that can
 * only traverse the decision tree once each time they run
 */

// TODO

////////////////////////////////////////////////////////////////////////////////

} // namespace tree_guide

#endif
