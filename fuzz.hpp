#define TARGET_PROGRAM "target/simple_instr"
#define CORPUS_DIR "corpus/"               // core corpus files
#define TEST_CASES_PATH "cases/"           // path where cases for fuzz cycle are dumped
#define DEBUG_LOG true
#define CRASHES_PATH "crashes/"            // detected crashes 

#define SEED 1337
#define SHARED_MEM_SIZE 1024

#define MIN_TRACE_CONTENT_SIZE 10         // minimal size for a sample to trigger crash

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

struct ExecTrace {
  uint64_t addr;
  int id;                                // used to uniquly identify and write to file
  int execution_trace[SHARED_MEM_SIZE];
  bool segfault;
  std::string content;
} typedef ExecTrace;

#endif

