#!/bin/bash

rm -rf out/*

# export AFL_DEBUG_CHILD=1
export FILEGUIDE_COMMENT_PREFIX=\;\ 
export FILEGUIDE_GENERATOR=$HOME/alive2-regehr/build/quick-fuzz
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/guided-tree-search/aflplusplus/guide-gen.so

$HOME/AFLplusplus/afl-fuzz -i ./in -o out -M f00 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &

export AFL_NO_UI=1
export AFL_QUIET=1

$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f01 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f02 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f03 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f04 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f05 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f06 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f07 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f08 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f09 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f10 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f11 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f12 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f13 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f14 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null &
$HOME/AFLplusplus/afl-fuzz -i ./in -o out -S f15 -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@ -o /dev/null
