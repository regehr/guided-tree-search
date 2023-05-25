# A COVERAGE-BASED GUIDE

- uses the stable branch from May 22 2023: https://github.com/AFLplusplus/AFLplusplus

- build using a command like this: `AFL=$HOME/AFLplusplus make`

- run using a command like this: `AFL_CUSTOM_MUTATOR_ONLY=1 AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/guided-tree-search/aflplusplus/guide-gen.so ~/AFLplusplus-regehr/afl-fuzz -i ./in -o out -- /home/regehr/llvm-project/build-for-afl/bin/opt -O2 @@`
