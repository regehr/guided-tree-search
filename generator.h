#ifndef UNIFORM_GENERATOR_H_
#define UNIFORM_GENERATOR_H_

#include <cassert>
#include <memory>
#include <random>
#include <vector>

namespace uniform {

// TODO put everything except Generator into a "details" namespace

struct Node;

struct Child {
  std::unique_ptr<Node> Ptr;
  long Weight;
};

struct Node {
  std::vector<Child> Children;
  std::unique_ptr<Node> Parent;
};

class Generator {
  std::unique_ptr<Node> Root;
  Node *Current;
  const long DefaultWeight = 10;
  std::random_device RD;
  std::unique_ptr<std::default_random_engine> Rand;

public:
  Generator() {
    auto seed = RD();
    Rand = std::make_unique<std::default_random_engine>(seed);
  }

  /*
   * start a traversal (terminating the one that was in progress, if
   * any); the next choose() will be at the top of the tree
   */
  inline void start();

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

void Generator::start() {
  Current = &*Root;
}

int Generator::choose(int n) {
  // FIXME avoid bias
  return (*Rand)() % n;
}

bool Generator::flip() { return choose(2); }

/*
 * the point of this class is to offer the naive alternative to the
 * smarter generator, as a basis for comparison 
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

} // namespace uniform

#endif
