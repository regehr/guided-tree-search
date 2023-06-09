extern "C" {
#include "afl-fuzz.h"
}

#include <sstream>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guide.h"
#include "mutate.h"

/////////////////////////////////////////////////////////////////////////////////////

static bool DEBUG_PLUGIN = false;

struct my_mutator {
  afl_state_t *afl;
  size_t trim_size_current;
  int trimmming_steps;
  int cur_step;
  u8 *mutated_out, *post_process_buf, *trim_buf;
};

std::string Prefix, Generator;

static std::string getEnvVar(std::string const &var) {
  char const *val = getenv(var.c_str());
  return (val == nullptr) ? std::string() : std::string(val);
}

extern "C" my_mutator *afl_custom_init(afl_state_t *afl, unsigned int seed) {
  Prefix = getEnvVar("FILEGUIDE_COMMENT_PREFIX");
  if (Prefix.empty()) {
    std::cerr << "\nERROR: Expected comment string in env var called "
                 "FILEGUIDE_COMMENT_PREFIX\n\n";
    exit(-1);
  }

  Generator = getEnvVar("FILEGUIDE_GENERATOR");
  if (Generator.empty()) {
    std::cerr << "\nERROR: Expected full path to generator in env var called "
                 "FILEGUIDE_GENERATOR\n\n";
    exit(-1);
  }

  my_mutator *data = (my_mutator *)calloc(1, sizeof(my_mutator));
  if (!data) {
    perror("afl_custom_init alloc");
    exit(-1);
  }

  if ((data->mutated_out = (u8 *)malloc(MAX_FILE)) == NULL) {
    perror("afl_custom_init malloc");
    exit(-1);
  }

  if ((data->post_process_buf = (u8 *)malloc(MAX_FILE)) == NULL) {
    perror("afl_custom_init malloc");
    exit(-1);
  }

  if ((data->trim_buf = (u8 *)malloc(MAX_FILE)) == NULL) {
    perror("afl_custom_init malloc");
    exit(-1);
  }

  data->afl = afl;
  return data;
}

//////////////////////////////////////////////////////////////////////////////

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
  tree_guide::FileGuide FG;
  FG.setSync(tree_guide::Sync::RESYNC);
  if (!FG.parseChoices(SS, Prefix)) {
    std::cerr << "ERROR: couldn't parse choices from:\n";
    std::cerr << SS.str();
    std::cerr << "--------------------------\n\n";
    exit(-1);
  }
  auto C1 = FG.getChoices();
  if (DEBUG_PLUGIN)
    std::cerr << "parsed " << C1.size() << " choices\n";
  mutator::mutate_choices(C1);
  if (DEBUG_PLUGIN)
    std::cerr << "mutated\n";
  FG.replaceChoices(C1);

  tree_guide::SaverGuide SG(&FG, Prefix);
  auto Ch = SG.makeChooser();
  auto Ch2 = static_cast<tree_guide::SaverChooser *>(Ch.get());
  assert(Ch2);

  std::string InFn(std::tmpnam(nullptr));
  std::string OutFn(std::tmpnam(nullptr));

  std::ofstream Outf(InFn, std::ios::binary);
  if (!Outf.is_open()) {
    std::cerr
      << "ERROR: mutator plugin could not save a file for the generator\n";
    exit(-1);
  }
  Outf << Prefix + tree_guide::StartMarker + "\n";
  Outf << Prefix;
  for (auto c : C1) {
    switch (c.k) {
    case tree_guide::RecKind::START:
      Outf << "{";
      break;
    case tree_guide::RecKind::END:
      Outf << "}";
      break;
    case tree_guide::RecKind::NUM:
      Outf << c.v;
      break;
    default:
      assert(false);
    }
    Outf << ",";
  }
  Outf << "\n" << Prefix + tree_guide::EndMarker + "\n";
  Outf.close();

  auto pid = fork();
  if (pid == -1) {
    std::cerr << "ERROR: fork failed\n";
    exit(-1);
  }
  if (pid == 0) {
    // child
    char *argv[] = {(char *)Generator.c_str(), nullptr};
    auto env1 = "FILEGUIDE_INPUT_FILE=" + InFn;
    auto env2 = "FILEGUIDE_OUTPUT_FILE=" + OutFn;
    char *envp[] = {(char *)env1.c_str(), (char *)env2.c_str(), nullptr};
    if (DEBUG_PLUGIN) {
      for (int i = 0; envp[i] != nullptr; ++i)
        std::cerr << envp[i] << " ";
      std::cerr << "\n";
    }
    auto res = execve(Generator.c_str(), argv, envp);
    // of course this line normally does not execute
    exit(res);
  }

  // parent
  int wstatus;
  waitpid(pid, &wstatus, 0);
  if (!WIFEXITED(wstatus)) {
    std::cerr << "ERROR: child exited abnormally\n";
    exit(-1);
  }
  auto ret = WEXITSTATUS(wstatus);
  if (ret != 0) {
    std::cerr << "ERROR: child did not return 0\n";
    exit(-1);
  }

  std::ifstream Inf(OutFn, std::ios::binary);
  if (!Inf.is_open()) {
    std::cerr << "ERROR: mutator plugin could not load file written by the "
      "generator\n";
    exit(-1);
  }
  Inf.read((char *)data->mutated_out, MAX_FILE);
  std::streamsize amount = Inf.gcount();
  Inf.close();

  std::remove(InFn.c_str());
  std::remove(OutFn.c_str());

  if (DEBUG_PLUGIN) {
    std::cerr << "buffer:\n";
    std::cerr << (char *)data->mutated_out;
    std::cerr << "\n\n";
  }

  *out_buf = data->mutated_out;
  return amount;
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
