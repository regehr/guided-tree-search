extern "C" {
#include "afl-fuzz.h"
}

#include <sstream>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guide.h"

/////////////////////////////////////////////////////////////////////////////////////
// TODO
// - use a better PRNG
// - put length checks in the afl hook
// - add a lot more mutations, especially nesting-aware ones

static void seedit(long seed) { srand(seed); }

static void change_one(std::vector<uint64_t> &C) {
  long x = rand() % C.size();
  long v = rand();
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
  size_t trim_size_current;
  int trimmming_steps;
  int cur_step;
  u8 *mutated_out, *post_process_buf, *trim_buf;
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

//////////////////////////////////////////////////////////////////////////////

const bool DEBUG = false;

// Vsevolod you'll need to change these two variables
const std::string Prefix("; ");
const std::string Generator("/home/regehr/alive2-regehr/build/quick-fuzz");

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
                                  size_t add_buf_size, // add_buf can be NULL
                                  size_t max_size) {
  std::string Str((char *)buf, buf_size);
  std::stringstream SS(Str);
  auto FG = new tree_guide::FileGuide;
  if (!FG->parseChoices(SS, Prefix)) {
    std::cerr << "ERROR: couldn't parse choices\n";
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

  tree_guide::SaverGuide SG(FG, Prefix);
  auto Ch = SG.makeChooser();
  auto Ch2 = static_cast<tree_guide::SaverChooser *>(Ch.get());
  assert(Ch2);

  std::string InFn(std::tmpnam(nullptr));
  std::string OutFn(std::tmpnam(nullptr));

  {
    std::ofstream Outf(InFn, std::ios::binary);
    if (!Outf.is_open()) {
      std::cerr
          << "ERROR: mutator plugin could not save a file for the generator\n";
      return 0;
    }
    Outf << Prefix + "FORMATTED CHOICES:\n";
    Outf << Prefix;
    for (auto c : C1) {
      Outf << c << ",";
    }
    Outf << "\n";
    Outf.close();
  }

  auto pid = fork();
  if (pid == -1) {
    std::cerr << "ERROR: fork failed\n";
    return 0;
  }
  if (pid == 0) {
    // child
    char *argv[] = {(char *)Generator.c_str(), nullptr};
    auto env1 = "FILEGUIDE_INPUT_FILE=" + InFn;
    auto env2 = "FILEGUIDE_OUTPUT_FILE=" + OutFn;
    char *envp[] = {(char *)env1.c_str(), (char *)env2.c_str(), nullptr};
    auto res = execve(Generator.c_str(), argv, envp);
    exit(res);
  }

  // parent
  int wstatus;
  waitpid(pid, &wstatus, 0);
  if (!WIFEXITED(wstatus)) {
    std::cerr << "ERROR: child exited abnormally\n";
    return 0;
  }
  auto ret = WEXITSTATUS(wstatus);
  if (ret != 0) {
    std::cerr << "ERROR: child did not return 0\n";
    return 0;
  }

  {
    std::ifstream Inf(OutFn, std::ios::binary);
    if (!Inf.is_open()) {
      std::cerr << "ERROR: mutator plugin could not load file written by the "
                   "generator\n";
      return 0;
    }
    Inf.read((char *)data->mutated_out, MAX_FILE);
    Inf.close();
  }

  std::remove(InFn.c_str());
  std::remove(OutFn.c_str());

  if (DEBUG) {
    std::cerr << "buffer:\n";
    std::cerr << (char *)data->mutated_out;
    std::cerr << "\n\n";
  }

  *out_buf = data->mutated_out;
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
