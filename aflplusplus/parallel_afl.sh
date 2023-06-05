#!/bin/bash

rm -rf out/*

export AFL_DEBUG_CHILD=1
export FILEGUIDE_COMMENT_PREFIX=\;\ 
export FILEGUIDE_GENERATOR=$HOME/alive2-regehr/build/quick-fuzz
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/guided-tree-search/aflplusplus/guide-gen.so
NPROC=`nproc`

CMD=$HOME/llvm-project/build-for-afl/bin/llc
CMDARGS='-mcpu=skx'

AFLARGS='-V 30 -i ./in -o out'

$HOME/AFLplusplus/afl-fuzz $AFLARGS -M f00 -- $CMD $ARGS @@ -o /dev/null &

export AFL_NO_UI=1
export AFL_QUIET=1

for (( i=1; i<$NPROC; i++ ))
do
    echo $i
    $HOME/AFLplusplus/afl-fuzz $AFLARGS -S f${i} -- $CMD $ARGS @@ -o /dev/null &
done


for (( i=0; i<$NPROC; i++ ))
do
      wait
done
