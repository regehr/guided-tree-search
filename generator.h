#ifndef UNIFORM_GENERATOR_H_
#define UNIFORM_GENERATOR_H_

#include <random>

namespace uniform {

class Generator {
public:
  /*
   * start a traversal; the next choose() will be at the top of
   * the tree
   */
  inline void start();

  /*
   * return a number in 0..n
   * take file and line as arguments?
   */
  inline int choose(int n);
  inline int choose_nofork(int n);
  inline int choose_noeffect(int n);
  
  /*
   * shorthand for choose(2)
   * take file and line as arguments?
   */
  inline bool flip();

  // TODO add a method for making a random choice that won't affect
  // future decisions, we can save space there; this is just an
  // optimization though
};

void Generator::start() {
}

}

bool flip() {
  return choose(2);
}

#endif
