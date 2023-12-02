
#define TARGET_PROGRAM "target/simple"
#define CORPUS_DIR "corpus/"
#define TEST_CASES_PATH "cases/"
#define DEBUG_LOG true
#define BREAKPOINT_FILE ".bp_list"
#define CRASHES_PATH "crashes/"

#define SEED 1337
#define N_BRANCHES 10
#define STORAGE_ID "/fuzz_map"

#ifndef FUZZ_DATA_STRUCTURES
#define FUZZ_DATA_STRUCTURES
#include <stdio.h>
#include <vector>
#include <string>

const uint8_t FLIP_ARRAY[] =  {1, 2, 4, 8, 16, 32, 64};
const int CYCLE_LENS[] =  {1, 2, 4, 8, 16, 32};
const int BLOCK_SIZES[] = {1, 2, 4, 8, 16, 32, 64};

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
  int id;
  //std::vector<uint64_t> execution_path;
  int execution_trace[N_BRANCHES];
  bool segfault;
  std::string content;
} typedef CrashType;

#endif

