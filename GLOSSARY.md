# Glossary

Here are some terms that we use with specific technical meanings, and
will try to use consistently throughout code and documentation:

* **Generator** - a function or program such as Csmith that makes a
    sequence of choices and eventually produces a value, a **test
    case**. In standard generative fuzzing scenarios, a generator's
    choices are made randomly, using manually tuned
    weights. Generators exist because without them, efforts to
    construct valid inputs to systems under test that have
    sophisticated validity constraints will almost always fail.  In
    other words, while there is a very large number of C++ files that
    are smaller than 10 KB, roughly 0% of 10 KB strings are valid C++
    files.

* **Test case** - a value that is produced by a **generator**.  A
    value is the input to one iteration of a testing loop that might,
    for example, attempt to crash a compiler.

* **Chooser** (TODO: Better name?) - core API object with a `choice`
    method. Generators invoke this method repeatedly in order to
    explore a single branch of the decision tree, on the way to
    producing a **test case**. A well-formed generator must be
    deterministic modulo the chooser: it must not make choices on its
    own.

* **Guide** - a core API object that acts as a source of **choosers**.
    The guide has global context and attempts to create choosers that
    will result in **test cases** with desirable aggregate properties
    such as being different from each other, being similar to existing
    test cases that are at the coverage frontier, etc.

* **Choice sequence** - a sequence of integers corresponding to the
    results of some sequence of calls to `choice` by some
    **generator**.

* **Static choice point** - some point (file and line number) in a
    **generator**'s program where `choice` is called.

* **Decision tree** - A **generator** embeds a decision tree that
    describes all **test cases** that it is capable of emitting. In
    the expected case, the decision tree is much too large to fully
    explore, so we are forced to sample its leaves.

* **Decision tree leaf** - When a generator reaches a leaf of its
    decision tree, then it has made all of the choices that need to be
    made in order to emit a **test case**
