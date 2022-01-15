class Generator {
public:
  /*
   * return a number in 0..n
   * take file and line as arguments?
   */
  int choose(int n);

  /*
   * shorthand for choose(2)
   * take file and line as arguments?
   */
  bool flip() {
    return choose(2);
  }

  /*
   * finish a traversal, the next choose() will start at the top of
   * the tree
   */
  void done();

  // TODO add a method for making a random choice that won't affect
  // future decisions, we can save space there; this is just an
  // optimization though
};
