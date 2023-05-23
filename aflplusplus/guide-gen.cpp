extern "C" {
#include "afl-fuzz.h"
}

#include <sstream>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "guide.h"

/////////////////////////////////////////////////////////////////////////////////////
// TODO
// - use a better PRNG
// - length checks in the afl hook
// - lots more mutations, including reducer ones

static void seedit(long seed) {
  srand(seed);
}

static void change_one(std::vector<uint64_t> &C) {
  long x = rand() % C.size();
  long v = rand();
  // std::cerr << "going with " <<v << " at index " << x << "\n";
  C.at(x) = v;
}

static void remove(std::vector<uint64_t> &C) {
  auto s = C.size();
  auto start_idx = rand() % s;
  auto end_idx = start_idx + 1 + rand() % 10;
  if (end_idx >= s)
    end_idx = s - 1;
  C.erase(C.begin() + start_idx, C.begin() + end_idx);
}

static void insert(std::vector<uint64_t> &C) {
  auto s = C.size();
  auto insert_idx = rand() % s;
  auto to_insert = 1 + rand() % 10;
  for (auto i = 0; i < to_insert; ++i)
    C.insert(C.begin() + insert_idx, rand());
}

static void mutate_choices(std::vector<uint64_t> &C) {
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

/////////////////////////////////////////////////////////////////////////////////////

struct my_mutator {
  afl_state_t *afl;
  size_t       trim_size_current;
  int          trimmming_steps;
  int          cur_step;
  u8          *mutated_out, *post_process_buf, *trim_buf;
};

extern "C" my_mutator *afl_custom_init(afl_state_t *afl, unsigned int seed) {
  seedit(seed);

  my_mutator *data = (my_mutator *)calloc(1, sizeof(my_mutator));
  if (!data) {
    perror("afl_custom_init alloc");
    return NULL;
  }

  if ((data->mutated_out = (u8 *)malloc(MAX_FILE)) == NULL) {
    perror("afl_custom_init malloc");
    return NULL;
  }

  if ((data->post_process_buf = (u8 *)malloc(MAX_FILE)) == NULL) {
    perror("afl_custom_init malloc");
    return NULL;
  }

  if ((data->trim_buf = (u8 *)malloc(MAX_FILE)) == NULL) {
    perror("afl_custom_init malloc");
    return NULL;
  }

  data->afl = afl;
  return data;
}

/////////////////////////////////////////////////////////////////////////////////////

static std::string gen(tree_guide::Chooser &C, long Depth);

static std::string _char(tree_guide::Chooser &C) {
  switch (C.choose(5)) {
  case 0:
    return "a";
  case 1:
    return "b";
  case 2:
    return "c";
  case 3:
    return "d";
  case 4:
    return ".";
  default:
    assert(false);
  }
}

static long num(tree_guide::Chooser &C, long min, long max) {
  return min + C.choose(max - min);
}

static std::string gen_helper(tree_guide::Chooser &C, long Depth) {
  --Depth;
  if (Depth == 0)
    return _char(C);
  switch (C.choose(11)) {
  case 0:
    return _char(C);
  case 1:
    return gen(C, Depth) + "|" + gen(C, Depth);
  case 2:
    return "(" + gen(C, Depth) + ")";
  case 3:
    return gen(C, Depth) + gen(C, Depth);
  case 4:
    return gen(C, Depth) + "?";
  case 5:
    return gen(C, Depth) + "*";
  case 6:
    return gen(C, Depth) + "+";
  case 7:
    return gen(C, Depth) + "{" + std::to_string(num(C, 1, 5)) + "}";
  case 8:
    return gen(C, Depth) + "{" + std::to_string(num(C, 1, 5)) + ",}";
  case 9:
    return gen(C, Depth) + "{," + std::to_string(num(C, 1, 5)) + "}";
  case 10: {
    auto N = num(C, 0, 5);
    return gen(C, Depth) + "{" + std::to_string(N) + "," +
           std::to_string(N + num(C, 0, 4)) + "}";
  }
  default:
    assert(false);
  }
}

static std::string gen(tree_guide::Chooser &C, long Depth) {
  C.beginScope();
  auto s = gen_helper(C, Depth);
  C.endScope();
  return s;
}

//////////////////////////////////////////////////////////////////////////////

const bool DEBUG = false;

/**
 * Perform custom mutations on a given input
 *
 * (Optional for now. Required in the future)
 *
 * @param[in] data pointer returned in afl_custom_init for this fuzz case
 * @param[in] buf Pointer to input data to be mutated
 * @param[in] buf_size Size of input data
 * @param[out] out_buf the buffer we will work on. we can reuse *buf. NULL on
 * error.
 * @param[in] add_buf Buffer containing the additional test case
 * @param[in] add_buf_size Size of the additional test case
 * @param[in] max_size Maximum size of the mutated output. The mutation must not
 *     produce data larger than max_size.
 * @return Size of the mutated output.
 */
extern "C" size_t afl_custom_fuzz(my_mutator *data, uint8_t *buf,
                                  size_t buf_size, u8 **out_buf,
                                  uint8_t *add_buf,
                                  size_t   add_buf_size,  // add_buf can be NULL
                                  size_t   max_size) {
  std::string Str((char *)buf, buf_size);
  std::stringstream SS(Str);
  auto FG = new tree_guide::FileGuide;
  if (!FG->parseChoices(SS)) {
    std::cerr << "couldn't parse choices\n";
    // FIXME do something better 
    return 0;
  }
  auto C1 = FG->getChoices();
  if (DEBUG)
    std::cerr << "parsed " << C1.size() << " choices\n";
  mutate_choices(C1);
  if (DEBUG)
    std::cerr << "mutated\n";
  FG->replaceChoices(C1);

  tree_guide::SaverGuide SG(FG);
  auto Ch = SG.makeChooser();
  auto Ch2 = static_cast<tree_guide::SaverChooser *>(Ch.get());
  assert(Ch2);

  const int RegexDepth = 6;
  auto S = gen(*Ch2, RegexDepth);
  if (DEBUG)
    std::cerr << "generated regex: '" << S << "'\n";

  // the mutated choices may contain extra elements, missing elements,
  // and out-of-range elements; this gets us a cleaned-up version
  auto C2 = Ch2->formatChoices();
  if (DEBUG)
    std::cerr << "formatted choices: '" << S << "'\n";
  
  // print the choice sequence into mutated_out
  strcpy((char *)data->mutated_out, C2.c_str());
  strcat((char *)data->mutated_out, S.c_str());

  if (DEBUG) {
    std::cerr << "buffer:\n";
    std::cerr << (char *)data->mutated_out;
    std::cerr << "\n\n";
  }
  
  *out_buf = data->mutated_out;
  // return mutated_size;
  return strlen((char *)data->mutated_out);
}

extern "C" int32_t afl_custom_init_trim(my_mutator *data, uint8_t *buf,
                                        size_t buf_size) {
  return 0;
}

extern "C" size_t afl_custom_trim(my_mutator *data, uint8_t **out_buf) {
  *out_buf = data->trim_buf;
  return data->trim_size_current;
}

extern "C" int32_t afl_custom_post_trim(my_mutator *data, int success) {
  return 0;
}

extern "C" size_t afl_custom_havoc_mutation(my_mutator *data, u8 *buf,
                                            size_t buf_size, u8 **out_buf,
                                            size_t max_size) {
  *out_buf = buf;
  return buf_size;
}

extern "C" uint8_t afl_custom_havoc_mutation_probability(my_mutator *data) {
  return 0;
}

extern "C" void afl_custom_deinit(my_mutator *data) {
  free(data->post_process_buf);
  free(data->mutated_out);
  free(data->trim_buf);
  free(data);
}
