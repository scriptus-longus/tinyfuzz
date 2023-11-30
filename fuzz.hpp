
#define TARGET_PROGRAM "target/simple"
#define CORPUS_DIR "corpus/"
#define TEST_CASES_PATH "cases/"
#define DEBUG_LOG true
#define BREAKPOINT_FILE ".bp_list"
#define CRASHES_PATH "crashes/"

#define SEED 1337

#ifndef FUZZ_DATA_STRUCTURES
#define FUZZ_DATA_STRUCTURES
#include <stdio.h>
#include <vector>
#include <string>

struct Breakpoint {
  uint32_t addr;
  uint64_t instr;
} typedef Breakpoint;

struct corpus_file {
  std::string filename;
  std::string content;
} typedef corpus_file;

struct Crash {
  uint64_t addr;
  std::vector<uint64_t> execution_path;
  bool segfault;
  std::string content;
} typedef CrashType;

#endif

