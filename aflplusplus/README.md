# A COVERAGE-BASED GUIDE

`AFL_CUSTOM_MUTATOR_ONLY=1 AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/AFLplusplus-regehr/custom_mutators/tree-guide-mutator/guide-gen.so $HOME/AFLplusplus-regehr/afl-fuzz -i in -o out -- $HOME/grep-3.11/build/src/grep -E -f @@ $HOME/svn/code/xorshift.c`
