# PROBLEM STATEMENT

# GOALS

- header-only, plugs into existing generators with little effort

- for small decision trees, rapidly converge to the desired behavior

- for huge ones, slow convergence is fine

- support weighted choices

- allow the user to customize behavior, e.g. don't insist that an
  integer constant in the output corresponds to 2^64 leaves

# TODO

- write a collection of trivial but pathological generators, such as
  the one that calls flip() 1000 times in succession

- work out how to evaluate how well this thing works, when used in
  non-trivial situations

- plug this into Csmith and YARPGen and see what happens

- work out how to take hints from the user about things like desirable
  patterns, subtrees that are likely to behave similarly every time we
  reach them, etc.

- work out how to take feedback about good/bad paths into account
