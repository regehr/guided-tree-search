#ifndef UNIFORM_GENERATOR_H_
#define UNIFORM_GENERATOR_H_

#include <cassert>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <optional>
#include <queue>

// TODO abstract base class for generator, so far we'll have three
// implementations: the naive one, the BFS+random one, and the one
// that implements a cardinality estimator

// TODO put everything except Generator into a "details" namespace?

// TODO perhaps have a virtual superclass if we're doing very many of
// these

// TODO once things are working, we can play allocator games to
// substantially reduce memory use

// TODO support weighted choice using an interface like YARPGen's
// internal one

// TODO wrap this library in a process and write a separate library
// that talks to it using RPC or whatever, so programs like Csmith can
// use this; perhaps use https://github.com/rpclib/rpclib

// TODO maybe take file name and line number as arguments to choose()
// functions -- this will support inference of self-similarity in the
// choice tree based on the fact that it's just the same code
// executing over and over (but with different inputs, so the
// self-similarity is not expected to be perfect)

// TODO support rejection of samples:
// https://github.com/regehr/uniform-tree-sampling/issues/2

// TODO optionally stop adding nodes to the explicit decision tree
// beyond a certain depth, since beyond a certain point we're just
// never going to be able to make use of the information contained
// down there

namespace uniform {

#ifdef _DEBUG
static const bool Debug = true;
#else
static const bool Debug = false;
#endif

// TODO just inline this file at some point
#include "priq.h"

static int TotalNodes = 0;

class Generator {
  struct Node {
    Node *Parent;
    std::vector<std::unique_ptr<Node>> Children;
    Node() { TotalNodes++; }
  };

  std::unique_ptr<Node> Root;
  Node *Current;
  int LastChoice;
  int Level;
  bool Started = false;
  std::random_device RD;
  std::unique_ptr<std::mt19937_64> Rand;
  // this vector is in reverse order so we can pop stuff efficiently
  std::vector<int> SavedChoices;

public:
  Generator() {
    Root = std::make_unique<Node>();
    Root->Children.resize(1);
    Rand = std::make_unique<std::mt19937_64>(RD());
  }

  /*
   * start a traversal (terminating the one that was in progress, if
   * any); the next choose() will be at the top of the tree
   */
  inline bool start();

  /*
   * return a number in 0..n
   */
  inline int choose(int n);

#if 0
  /*
   * adds n leaves but doesn't branch the tree; use this when this choice
   * does not affect any subsequent choices
   */
  inline int choose_nofork(int n);

  /*
   * generate a random integer without adding any leaves to the tree;
   * this this to generate things like strings and integer constants
   * where we don't want to sample the whole space
   */
  inline int choose_noeffect(int n);
#endif

  /*
   * shorthand for choose(2)
   */
  inline bool flip();
};

bool Generator::start() {
  // pick highest priority node off the priority Q
  // walk up to the root, saving the path to get back to this node
  // print something when we finish a level and assert that it doens't come back
  // return false if we're done exploring the tree
  //   or, just start sampling leaves uniformly
  if (Debug)
    std::cout << "*** START *** (total nodes = " << TotalNodes << ")\n";
  Current = &*Root;
  LastChoice = 0;
  Level = 0;
  Started = true;
  return true;
}

int Generator::choose(int Choices) {
  if (Debug)
    std::cout << "choose(" << Choices << ") at Level " << Level << "\n";
  assert(Started);

  int Choice;
  auto N = Current->Children.at(LastChoice).get();
  if (N) {
    /*
     * we've arrived at a tree node that has already been visited
     */
    if ((unsigned long)Choices != N->Children.size()) {
      // TODO it's unfriendly to exit here, but this is a critical API
      // violation and it's not clear how to proceed once it has
      // happened
      std::cout << "ERROR: Reached same node again, but different "
        "number of choices this time\n";
      exit(-1);
    }
    int NumSavedChoices = SavedChoices.size();
    assert(NumSavedChoices > 0);
    Choice = SavedChoices.at(NumSavedChoices - 1);
    SavedChoices.pop_back();
  } else {
    /*
     * we're off the beaten path, add this decision node to the tree
     * and make a random choice
     */
    assert(SavedChoices.size() == 0);
    N = new Node;
    N->Parent = Current;
    N->Children.resize(Choices);
    auto UN = std::unique_ptr<Node>(N);
    Current->Children.at(LastChoice) = std::move(UN);
    Current = N;
    std::uniform_int_distribution<int> Dist(0, Choices - 1);
    Choice = Dist(*Rand);
    // TODO add this node to the priority queue at its depth
  }
  LastChoice = Choice;
  Level++;
  if (Debug)
    std::cout << "  returning " << Choice << "\n";
  return Choice;
}

bool Generator::flip() { return choose(2); }

#if 0
/*
 * the point of this class is to offer the naive alternative to the
 * smarter generator, as a basis for comparison and so people can get
 * used to the API without the heavyweight path selection stuff going on
 */
class NaiveGenerator {
  std::random_device RD;
  std::unique_ptr<std::default_random_engine> Rand;

public:
  NaiveGenerator() {
    auto seed = RD();
    Rand = std::make_unique<std::default_random_engine>(seed);
  }
  inline void start() {}
  inline int choose(int n) {
    // FIXME avoid bias
    return (*Rand)() % n;
  }
  inline int choose_nofork(int n);
  inline int choose_noeffect(int n);
  inline bool flip() { return choose(2); }
};
#endif

} // namespace uniform

#endif
