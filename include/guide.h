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

#ifdef _DEBUG
static const bool Debug = false;
#else
static const bool Debug = false;
#endif

static const bool Verbose = false;

// TODO just inline this file at some point
#include "priq.h"

// TODO do we want to detect bad nesting of scopes?

// TODO make the basic type a size_t or something instead of long

////////////////////////////////////////////////////////////////////////////////

/*
 * abstract base class for all of the choosers
 */

class Chooser {
protected:
  Chooser() {}

public:
  virtual ~Chooser() {}
  // return a number in 0..n
  virtual long choose(long n) = 0;
  // shorthand for choose(2)
  virtual bool flip() = 0;
  // weighted choice
  virtual long chooseWeighted(const std::vector<double> &) = 0;
  virtual long chooseWeighted(const std::vector<long> &) = 0;
  /*
   * this call has a very specific contract: it does not cause the
   * decision tree to branch; it must only be used when the value that
   * is returned will not affect subsequent decisions made by the
   * generator. it might be used, for example, to generate a literal
   * constant in the output, or the name of an identifer
   */
  virtual long chooseUnimportant() = 0;
  virtual void beginScope() = 0;
  virtual void endScope() = 0;
};

// abstract base class for all of the guides
class Guide {
public:
  Guide() {}
  Guide(long) {}
  virtual ~Guide() {}
  virtual std::unique_ptr<Chooser> makeChooser() = 0;
  virtual const std::string name() = 0;
};

////////////////////////////////////////////////////////////////////////////////

/*
 * DefaultGuide: the point of this class is to offer the naive
 * alternative to the smarter generator, as a basis for comparison and
 * so people can get used to the API without the heavyweight path
 * selection stuff going on
 */

class DefaultGuide;

class DefaultChooser : public Chooser {
  DefaultGuide &G;

public:
  inline DefaultChooser(DefaultGuide &_G) : G(_G) {}
  inline ~DefaultChooser(){};
  inline long choose(long Choices) override;
  inline bool flip() override { return choose(2); }
  inline long chooseWeighted(const std::vector<double> &) override;
  inline long chooseWeighted(const std::vector<long> &) override;
  inline long chooseUnimportant() override;
  inline void beginScope() override{};
  inline void endScope() override{};
};

class DefaultGuide : public Guide {
  friend DefaultChooser;
  std::unique_ptr<std::mt19937_64> Rand;

public:
  inline DefaultGuide(long Seed) {
    Rand = std::make_unique<std::mt19937_64>(Seed);
  }
  inline DefaultGuide() : DefaultGuide(std::random_device{}()) {}
  inline ~DefaultGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override {
    return std::make_unique<DefaultChooser>(*this);
  }
  inline const std::string name() override { return "default"; }
};

long DefaultChooser::choose(long Choices) {
  std::uniform_int_distribution<int> Dist(0, Choices - 1);
  return Dist(*this->G.Rand);
}

long DefaultChooser::chooseWeighted(const std::vector<double> &Probs) {
  std::discrete_distribution<long> Discrete(Probs.begin(), Probs.end());
  return Discrete(*this->G.Rand);
}

long DefaultChooser::chooseWeighted(const std::vector<long> &Probs) {
  std::discrete_distribution<long> Discrete(Probs.begin(), Probs.end());
  return Discrete(*this->G.Rand);
}

long DefaultChooser::chooseUnimportant() {
  std::uniform_int_distribution<long> Dist(std::numeric_limits<long>::min(),
                                           std::numeric_limits<long>::max());
  return Dist(*this->G.Rand);
}

////////////////////////////////////////////////////////////////////////////////

/*
 * BFSGuide: exhaustive breadth-first exploration of the decision
 * tree, reverting to random choices once beyond the BFS frontier
 */

// TODO this guide uses way too much memory -- since it never frees,
// it shouldn't be too difficult to replace its allocated cells with a
// large flat allocation

class BFSChooser;

class BFSGuide : public Guide {
  friend BFSChooser;
  struct Node {
    Node *Parent;
    std::vector<std::unique_ptr<BFSGuide::Node>> Children;
  };

  long TotalNodes = 0;
  std::unique_ptr<BFSGuide::Node> Root;
  PriQ<Node *> PendingPaths;
  long MaxSavedLevel = -1;
  bool Choosing = false, Started = false;
  // TODO move this into the chooser?
  std::unique_ptr<std::mt19937_64> Rand;

public:
  inline BFSGuide(long Seed);
  inline BFSGuide() : BFSGuide(std::random_device{}()) {}
  inline ~BFSGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override;
  inline const std::string name() override { return "BFS"; }
};

class BFSChooser : public Chooser {
  friend BFSGuide;
  BFSGuide &G;
  BFSGuide::Node *Current;
  long LastChoice = 0, Level = 0;
  // this vector is in reverse order so we can pop stuff efficiently
  std::vector<long> SavedChoices;
  inline long chooseInternal(long, std::function<long()>);

public:
  inline BFSChooser(BFSGuide &_G) : G(_G) { Current = &*G.Root; }
  inline ~BFSChooser();
  inline long choose(long Choices) override;
  inline bool flip() override;
  inline long chooseWeighted(const std::vector<double> &) override;
  inline long chooseWeighted(const std::vector<long> &) override;
  inline long chooseUnimportant() override;
  inline void beginScope() override{};
  inline void endScope() override{};
};

BFSGuide::BFSGuide(long Seed) {
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
    assert(SavedLevel >= MaxSavedLevel);
    if (Verbose && SavedLevel > MaxSavedLevel)
      std::cout << "fully explored up to " << SavedLevel << "\n";
    MaxSavedLevel = SavedLevel;
    auto C = std::make_unique<BFSChooser>(*this);

    auto N = OptionalNode.value();
    BFSGuide::Node *N2 = nullptr;
    // this loop walks up to the root, saving the decisions that we
    // have to make to get back down here
    do {
      long Next = -1;
      long S = N->Children.size();
      if (N2) {
        // we're above the target node, so just get to the target
        for (long i = 0; i < S; ++i) {
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
        long NumUntaken = 0;
        for (long i = 0; i < S; ++i) {
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
      assert(Next != -1);
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

long BFSChooser::chooseInternal(const long Choices,
                                std::function<long()> randomChoice) {
  assert(G.Choosing);
  if (Verbose) {
    std::cout << "choose(" << Choices << ")\n";
    std::cout << "  Current = " << Current << ", LastChoice = " << LastChoice
              << "\n";
  }

  long Choice;
  auto N = Current->Children.at(LastChoice).get();
  if (Verbose)
    std::cout << "Node pointer = " << N << "\n";
  if (N) {
    /*
     * we've arrived at a tree node that has already been visited
     */
    if ((unsigned long)Choices != N->Children.size()) {
      // TODO it's unfriendly to exit here, but this is a critical API
      // violation. alternatively, of course we could throw an
      // exception
      std::cout << "FATAL ERROR: Reached same node again, but different "
                   "number of choices this time\n\n";
      exit(-1);
    }
    long NumSavedChoices = SavedChoices.size();
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

long BFSChooser::choose(long Choices) {
  return chooseInternal(Choices, [&]() -> long {
    std::uniform_int_distribution<int> Dist(0, Choices - 1);
    return Dist(*G.Rand);
  });
}

bool BFSChooser::flip() { return choose(2); }

long BFSChooser::chooseWeighted(const std::vector<double> &Probs) {
  return chooseInternal(Probs.size(), [&]() -> long {
    std::discrete_distribution<long> Discrete(Probs.begin(), Probs.end());
    return Discrete(*this->G.Rand);
  });
}

long BFSChooser::chooseWeighted(const std::vector<long> &Probs) {
  return chooseInternal(Probs.size(), [&]() -> long {
    std::discrete_distribution<long> Discrete(Probs.begin(), Probs.end());
    return Discrete(*this->G.Rand);
  });
}

long BFSChooser::chooseUnimportant() {
  std::uniform_int_distribution<long> Dist(std::numeric_limits<long>::min(),
                                           std::numeric_limits<long>::max());
  return Dist(*this->G.Rand);
}

////////////////////////////////////////////////////////////////////////////////

/*
 * tries to explore subtrees of the decision tree in an intelligent
 * fashion using techniques resembling cardinality estimation
 */

class WeightedSamplerChooser;

class WeightedSamplerGuide : public Guide {
  friend WeightedSamplerChooser;

  struct Node {
    bool visited = false;
    size_t BranchFactor;
    std::vector<double> Weights;
    std::unordered_map<long, std::unique_ptr<Node>> Children;
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
  inline WeightedSamplerGuide(long Seed) {
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

  inline long choose(long Choices, const std::vector<double> &Weights) {
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
         (current->Children.size() <= 5 || unif(*this->G.Rand) <= 0.1));

    if (explore) {
      if (current->Weights.size() > 0) {
        std::discrete_distribution<long> Dist(current->Weights.begin(),
                                              current->Weights.end());
        while (true) {
          result = Dist(*this->G.Rand);
          if (current->Children[result] == nullptr)
            break;
        }
      } else {
        std::uniform_int_distribution<long> Dist(0, current->BranchFactor - 1);
        while (true) {
          result = Dist(*this->G.Rand);
          if (current->Children[result] == nullptr)
            break;
        }
      }

      next_node = (current->Children[result] =
                       std::make_unique<WeightedSamplerGuide::Node>())
                      .get();

    } else {
      std::vector<long> results;
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

      auto i = Dist(*this->G.Rand);

      result = results[i];

      next_node = current->Children[result].get();
    }

    assert(next_node != nullptr);

    this->Trail.push_back(next_node);
    return result;
  };

  inline long choose(long Choices) override {
    std::vector<double> empty;
    return this->choose(Choices, empty);
  }

  inline bool flip() override { return choose(2); }
  inline long chooseWeighted(const std::vector<double> &) override;
  inline long chooseWeighted(const std::vector<long> &) override;
  inline long chooseUnimportant() override;
  inline void beginScope() override{};
  inline void endScope() override{};
};

std::unique_ptr<Chooser> WeightedSamplerGuide::makeChooser() {
  return std::make_unique<WeightedSamplerChooser>(*this);
}

long WeightedSamplerChooser::chooseWeighted(const std::vector<double> &Probs) {
  return this->choose(Probs.size(), Probs);
}

long WeightedSamplerChooser::chooseWeighted(const std::vector<long> &Probs) {
  long Total = 0;
  for (auto I : Probs)
    Total += I;
  std::vector<double> V;
  for (auto I : Probs)
    V.push_back((double)I / Total);
  return this->choose(Probs.size(), V);
}

long WeightedSamplerChooser::chooseUnimportant() {
  std::uniform_int_distribution<long> Dist(std::numeric_limits<long>::min(),
                                           std::numeric_limits<long>::max());
  return Dist(*this->G.Rand);
}

////////////////////////////////////////////////////////////////////////////////

/*
 * SaverGuide: wraps another guide in order to remember choices that
 * it made; use the chooser's getChoices() or formatChoices() methods
 * to retreive them
 */

template <typename T> class SaverChooser;

template <typename T> class SaverGuide : public Guide {
  friend SaverChooser<T>;
  std::unique_ptr<DefaultGuide> DG;
  const size_t MAX_LINE_LENGTH = 70;

public:
  inline SaverGuide(long Seed) { DG = std::make_unique<DefaultGuide>(Seed); }
  inline SaverGuide() { DG = std::make_unique<DefaultGuide>(); }
  inline ~SaverGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override {
    return std::make_unique<SaverChooser<T>>(*this);
  }
  inline const std::string name() override {
    return DG->name() + " (wrapped by Saver)";
  }
};

template <typename T> class SaverChooser : public Chooser {

  enum kind { START = 777, END, NUM, NONE };
  struct rec {
    kind k;
    long v;
  };

  SaverGuide<T> &G;
  std::unique_ptr<Chooser> C;
  std::vector<rec> Saved;

public:
  inline SaverChooser(SaverGuide<T> &_G) : G(_G) { C = G.DG->makeChooser(); }
  inline ~SaverChooser(){};
  inline long choose(long Choices) override;
  inline bool flip() override { return choose(2); }
  inline long chooseWeighted(const std::vector<double> &) override;
  inline long chooseWeighted(const std::vector<long> &) override;
  inline long chooseUnimportant() override;
  inline const std::vector<long> &getChoices();
  inline const std::string formatChoices();
  inline void beginScope() override;
  inline void endScope() override;
};

template <typename T> long SaverChooser<T>::choose(long Choices) {
  auto X = C->choose(Choices);
  rec r{NUM, X};
  Saved.push_back(r);
  return X;
}

template <typename T>
long SaverChooser<T>::chooseWeighted(const std::vector<double> &Probs) {
  auto X = C->chooseWeighted(Probs);
  rec r{NUM, X};
  Saved.push_back(r);
  return X;
}

template <typename T>
long SaverChooser<T>::chooseWeighted(const std::vector<long> &Probs) {
  auto X = C->chooseWeighted(Probs);
  rec r{NUM, X};
  Saved.push_back(r);
  return X;
}

template <typename T> long SaverChooser<T>::chooseUnimportant() {
  auto X = C->chooseUnimportant();
  rec r{NUM, X};
  Saved.push_back(r);
  return X;
}

template <typename T> void SaverChooser<T>::beginScope() {
  rec r{START, 0};
  Saved.push_back(r);
  C->beginScope();
}

template <typename T> void SaverChooser<T>::endScope() {
  rec r{END, 0};
  Saved.push_back(r);
  C->endScope();
}

// NB the vector referenced by the return value here's lifetime will
// end when the chooser's lifetime ends
template <typename T> const std::vector<long> &SaverChooser<T>::getChoices() {
  return Saved;
}

template <typename T> const std::string SaverChooser<T>::formatChoices() {
  std::string s;
  s += "/*\n * FORMATTED CHOICES:\n";
  std::vector<long>::size_type pos = 0;
  std::string line = " * ";
  while (pos < Saved.size()) {
    std::string item;
    switch (Saved.at(pos).k) {
    case START:
      item = "{";
      break;
    case END:
      item = "}";
      break;
    case NUM:
      item = std::to_string(Saved.at(pos).v);
      break;
    default:
      assert(false);
    }
    item += ",";
    if (line.length() + item.length() >= G.MAX_LINE_LENGTH) {
      s += line + "\n";
      line = " * " + item;
    } else {
      line += item;
    }
    ++pos;
  }
  s += line + "\n";
  s += " */\n";
  return s;
}

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
  inline RRGuide(long Seed) = delete;
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
  inline ~RRChooser(){};
  inline long choose(long Choices) override;
  inline bool flip() override { return choose(2); }
  inline long chooseWeighted(const std::vector<double> &) override;
  inline long chooseWeighted(const std::vector<long> &) override;
  inline long chooseUnimportant() override;
  inline bool hasSubChooser() { return C != nullptr; }
  inline void beginScope() override { C->beginScope(); };
  inline void endScope() override { C->endScope(); };
};

long RRChooser::choose(long Choices) { return C->choose(Choices); }

long RRChooser::chooseWeighted(const std::vector<double> &Probs) {
  return C->chooseWeighted(Probs);
}

long RRChooser::chooseWeighted(const std::vector<long> &Probs) {
  return C->chooseWeighted(Probs);
}

long RRChooser::chooseUnimportant() { return C->chooseUnimportant(); }

std::unique_ptr<Chooser> RRGuide::makeChooser() {
  auto C = std::make_unique<RRChooser>(*this);
  if (C->hasSubChooser())
    return C;
  else
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

/*
 * FileGuide: loads a file of choices; every chooser that it returns
 * does exactly the same thing: makes the choices specified in the
 * file
 */

class FileGuide;

class FileChooser : public Chooser {
  FileGuide &G;
  std::vector<long>::size_type Pos = 0;
  long Counter = 0;

public:
  inline FileChooser(FileGuide &_G) : G(_G) {}
  inline ~FileChooser(){};
  inline long choose(long Choices) override;
  inline bool flip() override { return choose(2); }
  inline long chooseWeighted(const std::vector<double> &) override;
  inline long chooseWeighted(const std::vector<long> &) override;
  inline long chooseUnimportant() override;
  inline void beginScope() override{};
  inline void endScope() override{};
};

class FileGuide : public Guide {
  friend FileChooser;
  std::vector<long> Choices;
  const std::string StartMarker = "* FORMATTED CHOICES:";
  const std::string EndMarker = "*/";
  enum kind { START = 777, END, NUM, NONE };

public:
  inline FileGuide(long Seed) = delete;
  inline FileGuide() = delete;
  inline FileGuide(std::string);
  inline ~FileGuide() {}
  inline std::unique_ptr<Chooser> makeChooser() override {
    return std::make_unique<FileChooser>(*this);
  }
  inline const std::string name() override { return "file"; }
};

FileGuide::FileGuide(std::string FileName) {
  std::ifstream file(FileName);
  if (!file.is_open()) {
    std::cerr << "FATAL ERROR: Cannot open choice file '" << FileName
              << "'\n\n";
    exit(-1);
  }
  std::string line;
  bool inData = false;
  while (std::getline(file, line)) {
    if (inData) {
      if (line.find(EndMarker) != std::string::npos) {
        break;
      } else {
        if (line[0] != ' ' || line[1] != '*' || line[2] != ' ') {
          std::cerr << "FATAL ERROR: Expected every line of choices in '"
                    << FileName << "' to start with ' * '\n\n";
          exit(-1);
        }
        long val = 0;
        kind k = NONE;
        for (std::string::size_type pos = 3; pos < line.length(); ++pos) {
          auto c = line[pos];
          if (c == ',') {
            switch (k) {
            case NUM:
              Choices.push_back(val);
              val = 0;
              break;
            case START:
              break;
            case END:
              break;
            default:
              assert(false);
            }
            k = NONE;
          } else if (c >= '0' && c <= '9') {
            val *= 10;
            val += c - '0';
            k = NUM;
          } else if (c == '{') {
            k = START;
          } else if (c == '}') {
            k = END;
          } else {
            std::cerr << "FATAL ERROR: Illegal character '" << c
                      << "' in choice string in '" << FileName << "'\n\n";
            exit(-1);
          }
        }
      }
    } else {
      if (line.find(StartMarker) != std::string::npos) {
        inData = true;
      }
    }
  }
  file.close();
  if (Choices.size() == 0) {
    std::cerr << "FATAL ERROR: The file '" << FileName
              << "' contained no choices\n\n";
    exit(-1);
  }
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

long FileChooser::choose(long Choices) {
  long val = (Pos < G.Choices.size()) ? G.Choices.at(Pos++) : Counter++;
  return val % Choices;
}

long FileChooser::chooseWeighted(const std::vector<double> &Probs) {
  long val = (Pos < G.Choices.size()) ? G.Choices.at(Pos++) : Counter++;
  return val % Probs.size();
}

long FileChooser::chooseWeighted(const std::vector<long> &Probs) {
  long val = (Pos < G.Choices.size()) ? G.Choices.at(Pos++) : Counter++;
  return val % Probs.size();
}

long FileChooser::chooseUnimportant() {
  return (Pos < G.Choices.size()) ? G.Choices.at(Pos++) : Counter++;
}

////////////////////////////////////////////////////////////////////////////////

/*
 * remote guide: ephemeral in-process guide that talks to a different
 * guide living in a server process; use this for generators that can
 * only traverse the decision tree once each time they run
 */

// TODO

////////////////////////////////////////////////////////////////////////////////

/*
 * coverage guide: takes feedback from afl++
 */

// TODO

////////////////////////////////////////////////////////////////////////////////

} // namespace tree_guide

#endif
