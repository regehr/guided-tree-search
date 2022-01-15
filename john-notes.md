Maciver simple impl   
  https://gist.github.com/DRMacIver/eb0e151834dd85f23659f6c2040fd6c9

hard thing: how to treat multiple occurrences of the same program point in the tree
  extreme 1: they're distinct
  extreme 2: they're all the same
  possible answer: let the user tell us
  ...

adaptive sampling for totally unknown tree structure
  another constraint: can't back up the tree
  does it provide useful convergence when samples << nodes?
  is this strategy optimal when we have no knowledge of tree structure?
  work out the API between this and randprog
  evaluate effect of MIN_VISITS on memory usage and convergence speed
  we want to be able to merge trees from different machines, perhaps
    sanity-checking those that come from untrusted souces
  we can hand promising sub-trees to other processors, giving them a
    fixed prefix, then merge the results back into the main tree

figure out how to exploit the kind of structure found in randprog or
tosmc where choice points are embedded in the code
  could assume generator is a CFG by taking, at each node, the maximum
    possible number of choices-- probably need to adjust randprog to
    meet this constraint, for example by limiting total number of jump
    targets, etc.
  amount of deviation from CFG is average deviation from maximum?
  can explore as if we had a CFG, but bias away from nodes that
    deviate more from their maxima

only represent a bounded number of nodes

deal specially with non-structural choices (values of integer constants)
  that don't affect future decisions

https://github.com/maciej-bendkowski/boltzmann-brain

have a "be careful what you ask for" section
  look at outputs and iterate
  https://gist.github.com/jorendorff/502c24cf3d1c4724b1f358208fcde96a
  funnel shift dominance
  constants
  variable names
  always measure!

need to be able to compare to monte carlo tree search, probably
see old notes in other-reserach.txt
https://hal-ens-lyon.archives-ouvertes.fr/ensl-00979074v2/document
https://arxiv.org/pdf/1607.05443.pdf
http://web.mit.edu/~ezyang/Public/p61-canou.pdf
what if uniform sampling makes things worse?
  https://twitter.com/jorendorff/status/1191421447027253249
  numeric constants, etc.
  real problem being solved is avoiding low-probability regions, may require more tweaks
http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.32.8707&rep=rep1&type=pdf
enumerate and select
easy algorithm if know subtree sizes
  https://www.math.upenn.edu/~wilf/website/Method%20and%20two%20algorithms.pdf
  this is similar to context free grammar case, I think
  Uniform Random Generation of Strings in a Context-Free Language
simulate full tree
  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.107.7731&rep=rep1&type=pdf
same, with early exit
random walk
??
DSL + tree flattening (similar to sampling from the full tree?)
  works best when series of choices are independent?
clearly there are limits when we know nothing about the tree
https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.496.1203&rep=rep1&type=pdf
approximate analytical solution
  validate for small tree depths
easy online algorithm: steer away from collisions
wanted: an online algorithm that learns the tree shape
  hard part is finding exploitable symmetries, will need user help
https://en.wikipedia.org/wiki/Sobol_sequence

