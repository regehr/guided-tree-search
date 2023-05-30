#include "guide.h"
#include "mutate.h"

/////////////////////////////////////////////////////////////////////////////////////
// TODO
// - use a better PRNG
// - add a lot more mutations, especially nesting-aware ones

void seedit(long seed) { srand(seed); }

static void change_one(std::vector<tree_guide::rec> &C) {
  long x;
  do {
    x = rand() % C.size();
  } while (C.at(x).k != tree_guide::NUM);
  long v = rand();
  C.at(x).v = v;
}

static void remove(std::vector<tree_guide::rec> &C) {
  auto s = C.size();
  auto start_idx = rand() % s;
  auto end_idx = start_idx + 1 + rand() % 10;
  if (end_idx >= s)
    end_idx = s - 1;
  C.erase(C.begin() + start_idx, C.begin() + end_idx);
}

static void insert(std::vector<tree_guide::rec> &C) {
  auto s = C.size();
  auto insert_idx = rand() % s;
  auto to_insert = 1 + rand() % 10;
  for (auto i = 0; i < to_insert; ++i) {
    tree_guide::rec r{ tree_guide::NUM, (uint64_t)rand() };
    C.insert(C.begin() + insert_idx, r);
  }
}

void mutate_choices(std::vector<tree_guide::rec> &C) {
  switch (rand() % 3) {
  case 0:
    change_one(C);
    break;
  case 1:
    remove(C);
    break;
  case 2:
    insert(C);
    break;
  }
}

