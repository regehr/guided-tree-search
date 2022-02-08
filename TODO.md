# TODO

- write some more generators in the test driver

- work out how to evaluate how well this thing works, when used in
  non-trivial situations

- coverage-guided generation

- plug this into Csmith and YARPGen and see what happens

- work out how to take hints from the user about things like desirable
  patterns, subtrees that are likely to behave similarly every time we
  reach them, etc.

- work out how to take feedback about good/bad paths into account

- maybe split this into multiple files but then put them together
  using a script so in the end there's a single file that people can
  use

- make BFS optionally sometimes sample from lower depths, or even
  round-robin among levels with available work

- probably should look at the priority queue in the middle of some
  runs and see how big the buckets are getting

- make everything here consistent with GLOSSARY.md

- cardinality estimator guide

- coverage-driven guide

- meta-guide that round-robins among existing ones

- support hierarchy/grouping in the stream of choices

- put everything except Guide into a "details" namespace?

- once things are working, we can play allocator games to
  substantially reduce memory use

- support weighted choice using an interface like YARPGen's
  qinternal one

- wrap this library in a process and write a separate library that
  talks to it using RPC or whatever, so programs like Csmith can use
  this; perhaps use https://github.com/rpclib/rpclib

- maybe take file name and line number as arguments to choose()
  functions -- this will support inference of self-similarity in the
  choice tree based on the fact that it's just the same code executing
  over and over (but with different inputs, so the self-similarity is
  not expected to be perfect)

- support rejection of samples:
  https://github.com/regehr/uniform-tree-sampling/issues/2

- optionally stop adding nodes to the explicit decision tree beyond a
  certain depth, since beyond a certain point we're just never going
  to be able to make use of the information contained down there

- what do we do about swarm testing, or parameter shuffling as YARPGen
  calls it?

