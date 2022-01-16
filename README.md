# PROBLEM STATEMENT

When write a randomized generator for some file format in a
general-purpose programming language, we can view the resulting
program as embedding a decision tree. When there are interesting
validity constraints on the generated file format, the decision trees
tend to be highly lopsided: some branches lead to enormous subtrees,
while others terminate rapidly.

When a generator that embeds a lopsided decision tree makes decisions
in the most obvious manner (every choice point picks among the
available alternatives with uniform random probability), some parts of
the tree will end up being explored a huge number of times whereas
others will be neglected.

As a simple example, consider a generator that embeds this decision
tree:

```
      N
     / \
    T1  N
       / \
      T2  N
         / \
        T3  N
           / \
          T4  N
             / \
            T5  N
               / \
              T6  N
                 / \
                T7  T8
         
```

Here Ns denote internal nodes and Ts are leaves in the decision tree;
reaching a leaf indicates that a complete test case has been
generated. Uniform random choices will, of course, cause T1 to be
generated with probability 1/2 whereas T7 and T8 will appear with
probability 1/128. In realistic situations, we have observed that
there are interesting outputs that are effectively impossible to
generate.

The problem statement, then, is to automatically adjust probabilities
in such a way that T1..T8 are reached with uniform probability. Of
course uniform probabilities are unlikely to be the optimal ones, but
our hypothesis is that uniform sample is, in general, much better than
unknown, difficult-to-fix biases. You should view uniform
probabilities as only a starting point for further tuning.

For certain well-structured special cases, such as generating strings
from a regular grammar, algorithms exist to generate strings with
uniform probability, but it is easy to see that in the general case,
no such algorithm can exist. The proof of this is handwavy, but
observe that an arbitrary-sized subtree can lurk past every unexplored
edge in the decision tree. Without prior knowledge of the tree shape,
there is simply no way to know which unexplored branches lead to one
or two leaves and which lead to a near-infinite number of leaves.

## A Solution

If we walk the decision tree systematically, instead of randomly, and
generate all possible outputs, then it becomes trivial to select from
them with uniform probability. The problem with this solution is that
most of the decision trees that we care about in practice are too
large for this to be feasible.

## Another Solution

If we can inspect a decision tree in its entirety, we can assign a
weight of 1 to every leaf node and then propagate these weights
upwards, where the weight of every internal node is the number of
leaves in that node's subtree. Then, we can generate leaves with
uniform probability by using node weights to bias random choices.
This solution also fails when the decision tree is too large.

FIXME cite

## Yet Another Solution

If we know the maximum depth of the decision tree, and the maximum
degree of any node in the decision tree, we can use rejection sampling
to reach every leaf with uniform probability. We simply pretend that
every node has the maximum degree, select from the choices using
uniform probabilities, and then start over every time we reach a
non-existent choice. (We also have to account for leaves that are
higher than the maximum tree depth, but this is straightforward.) The
problem with this solution is that realistic decision trees are
sparse; we will end up spending the vast majority of the available CPU
time rejecting samples.

FIXME cite

## Our Solution

Given a decision tree of unknown shape, we explore it in order to
infer its shape. The goal is to estimate the cardinality of every
subtree, particularly those that contain unexplored branches, so that
we can bias subsequent tree traversals towards parts of the tree that
appear to require the most exploration. In this fashion we can,
hopefully, approach the desired distribution of generated outputs,
after performing a potentially large number of tree traversals.

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
