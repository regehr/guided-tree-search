# A COVERAGE-BASED GUIDE

- tested against the afl++ stable branch from May 30 2023:
  https://github.com/AFLplusplus/AFLplusplus

- build by running a command like this: `AFL=$HOME/AFLplusplus make`

- run using a command like this: `AFL_CUSTOM_MUTATOR_ONLY=1 AFL_CUSTOM_MUTATOR_LIBRARY=$HOME/guided-tree-search/aflplusplus/guide-gen.so ~/AFLplusplus-regehr/afl-fuzz -i ./in -o out -- /home/regehr/llvm-project/build-for-afl/bin/opt -O2 @@`

## A Complete Example

1.

Build this version of Alive2, following the standard instructions
for building Alive2:

```
https://github.com/regehr/alive2/tree/use-tree-guide
```

The executable that you need here is called `quick-fuzz`.

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

Be warned that this LLVM build will be much slower than normal,
presumably due to the AFL instrumentation.

3.

