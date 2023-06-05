#!/bin/bash

rm -rf out*

export AFL_DEBUG_CHILD=1
export FILEGUIDE_COMMENT_PREFIX=\;\ 
export FILEGUIDE_GENERATOR=$HOME/alive2-regehr/build/quick-fuzz
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/guided-tree-search/aflplusplus/guide-gen.so
NPROC=`nproc`

CMD=$HOME/llvm-project/build-for-afl/bin/llc
CMDARGS='-mcpu=skx'

AFLARGS='-V 86400 -i ./in'

export AFL_NO_UI=1
export AFL_QUIET=1

for (( i=0; i<$NPROC; i++ ))
do
    echo $i
    mkdir out${n}
    $HOME/AFLplusplus/afl-fuzz $AFLARGS -o out${i} -- $CMD $ARGS @@ -o /dev/null &
done


for (( i=0; i<$NPROC; i++ ))
do
      wait
done
