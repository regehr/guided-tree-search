#ifndef UNIFORM_GENERATOR_H_
#define UNIFORM_GENERATOR_H_

// FIXME delete
#include <iostream>

#include <cassert>
#include <memory>
#include <random>
#include <vector>

#include "priq.h"

namespace uniform {

// TODO abstract base class for generator

// TODO put everything except Generator into a "details" namespace?

// TODO perhaps have a virtual superclass if we're doing very many of
// these

// TODO once things are working, we can play allocator games to
// substantially reduce memory use

static int TotalNodes = 0;

class Generator {
  struct Node {
    Node *Parent;
    std::vector<std::unique_ptr<Node>> Children;
    Node() { TotalNodes++; }
  };

  std::unique_ptr<Node> Root;
  Node *Current;
  int LastChoice = 0;
  std::random_device RD;
  std::unique_ptr<std::mt19937_64> Rand;

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

  /*
   * shorthand for choose(2)
   */
  inline bool flip();

  // TODO maybe take file name and line number as arguments to
  // choose() functions

  // TODO support rejection of samples:
  // https://github.com/regehr/uniform-tree-sampling/issues/2
};

bool Generator::start() {
  // pick highest priority node off the priority Q
  // walk up to the root, saving the path to get back to this node
  // print something when we finish a level and assert that it doens't come back
  // return false if we're done exploring the tree
  //   or, just start sampling leaves uniformly
  std::cout << "*** START *** (total nodes = " << TotalNodes << ")\n";
  Current = &*Root;
  LastChoice = 0;
  return true;
}

int Generator::choose(int Choices) {
  std::cout << "choose(" << Choices << ")\n";
  // we're either following a predetermined path, or we're past that
  // and just making random choices
  if (false) { // is there something in the path?
    // node transition
    // check that number of choices hasn't changed
    // return the predetermined choice
  } else {
    auto N = Current->Children.at(LastChoice).get();
    // FIXME this conditional not needed once the rest of this code works
    if (!N) {
      N = new Node;
      N->Parent = Current;
      N->Children.resize(Choices);
      auto UN = std::unique_ptr<Node>(N);
      Current->Children.at(LastChoice) = std::move(UN);
    }
    Current = N;
    std::uniform_int_distribution<int> Dist(0, Choices - 1);
    int Choice = Dist(*Rand);
    LastChoice = Choice;
    std::cout << "  returning " << Choice << "\n";
    return Choice;
    // TODO add this node to the priority queue at its depth
  }
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
