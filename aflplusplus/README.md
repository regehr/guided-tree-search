# A COVERAGE-BASED GUIDE

- tested against the afl++ stable branch from May 30 2023:
  https://github.com/AFLplusplus/AFLplusplus

## A Complete Example

The commands below are just examples, you'll likely need to adapt some
of them to point to the correct directories for you environment. This
has been tested only on Linux.

1. Build this version of Alive2, following the standard instructions
   for building Alive2:

```
https://github.com/regehr/alive2/tree/use-tree-guide
```

The executable that you need here is `quick-fuzz`

2. Build an LLVM with AFL coverage. For ambitious LLVM configurations
   we have seen problems where the AFL compiler driver fails due to
   creating command lines with more than 1024 arguments, so keep this
   simple. This LLVM configuration worked for us, using their main
   branch from May 30 2023. Starting in the root directory of the
   cloned LLVM repository:

```
export AFL_HOME=$HOME/AFLplusplus
mkdir build-for-afl
cd build-for-afl
cmake -GNinja -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="llvm" ../llvm -DBUILTINS_CMAKE_ARGS=-DCOMPILER_RT_ENABLE_IOS=OFF -DCMAKE_C_COMPILER=$AFL_HOME/afl-cc -DCMAKE_CXX_COMPILER=$AFL_HOME/afl-c++ -DLLVM_TARGETS_TO_BUILD=X86
ninja
```

Be warned that this LLVM build will be slower than normal, presumably
due to the AFL instrumentation.

3. Build the AFL custom mutator. From `guided-tree-search/aflplusplus`:

```
AFL=$HOME/AFLplusplus make
```

4. In a scratch directory make subdirectories called `in` and
  `out`. In the `in` directory make a file containing these contents:

```
; FORMATTED CHOICES:
; 1,0,3,4,0,1,0,1,0,3,0,0,0,2,0,1,0,3,0,1,2,3,0,2,3,3,2,3,7,1,0,10,0,
; 3,28,1,0,3,2,5,4,0,6,1,3,9,0,1,12,1,4,0,
```

The `out` directory should be empty.

5. Run AFL++:

```
FILEGUIDE_COMMENT_PREFIX=\;\  FILEGUIDE_GENERATOR=$HOME/alive2-regehr/build/quick-fuzz AFL_CUSTOM_MUTATOR_ONLY=1 AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/guided-tree-search/aflplusplus/guide-gen.so $HOME/AFLplusplus/afl-fuzz -i ./in -o out -- $HOME/llvm-project/build-for-afl/bin/opt -O3 @@
```
