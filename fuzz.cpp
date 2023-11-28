#include <stdio.h>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <sys/personality.h>
#include <algorithm>
#include <cmath>
#include <tuple>
#include "tracer.cpp"
#include "mutator.cpp"

// parameters
#define TARGET_PROGRAM "target/simple"
#define CORPUS_DIR "corpus/"
#define TEST_CASES_PATH "cases/"
#define DEBUG_LOG true
#define BREAKPOINT_FILE ".bp_list"

#define SEED 1337

/*
typedef void (*mutation_func) (std::string &data, uint32_t idx);

const uint8_t FLIP_ARRAY[] =  {1, 2, 4, 8, 16, 32, 64};
const int CYCLE_LENS[] =  {1, 2, 4, 8, 16, 32};
const int BLOCK_SIZES[] = {1, 2, 4, 8, 16, 32, 64};
*/
/*struct corpus_file {
  std::string filename;
  std::string content;
} typedef corpus_file;*/


std::vector<uint64_t> known_addresses;
std::vector<Breakpoint> breakpoints;

std::string cat_path(std::string path, std::string filename) {
  if (path.back() != '/') {
    path.append("/");
  }
  
  path.append(filename);
  return path;
}

std::string read_file(std::string path) {
  std::ifstream file_stream(path);
  std::stringstream buffer;
  std::string ret;


  buffer << file_stream.rdbuf();
  ret = buffer.str();

  file_stream.close();
  return ret;
}

void bit_flip(std::string &data, uint32_t idx) {
  uint8_t bit = FLIP_ARRAY[rand() % 7];
  data[idx] = data[idx] ^ bit; 
}

void byte_flip(std::string &data, uint32_t idx) {
  uint8_t byte =  rand() % 0xff; 
  data[idx] = data[idx] ^ byte; 
}

void add_block(std::string &data, uint32_t idx) {
  std::string block;
  uint8_t byte;
  int size = BLOCK_SIZES[rand() % 7];

  for (int i = 0; i < size; i++) {
    uint8_t byte =  rand() % 0xff; 
    block.push_back((char)byte);
  }

  data.insert(idx, block);
}

void remove_block(std::string &data, uint32_t idx) {
  int size_idx = rand() % 7;

  while (idx + BLOCK_SIZES[size_idx] > data.size()-1) {
    size_idx -= 1;
    if (size_idx < 0) {
      return;
    }
  }

  data.erase(idx, BLOCK_SIZES[size_idx]);
}

std::string mutate_data(std::string data) {
  uint32_t n = CYCLE_LENS[rand() % 6];
  std::string ret = data; 

  mutation_func mutations[] = {bit_flip, byte_flip, add_block, remove_block};

  for (uint32_t i = 0; i < n; i++) {
    uint32_t idx = (uint32_t)(rand() % ret.size());
    auto mutation = mutations[rand() % 4];
 
    mutation(ret, idx);

  }
  return ret;
}

uint64_t find_base_addr(uint64_t pid, std::string program_name) {
  std::stringstream path;
  path << "/proc/" << pid << "/maps";
  std::cout << path.str() << '\n'; 
  std::ifstream map(path.str());

  uint64_t ret = 0; 
 
  std::string line;
  while (std::getline(map, line)) { 
    //std::cout << segmentd_line.size() << '\n';
    if (line.size() < 74) {
      continue;
    } 
    if (line[25] != ' ') {
      continue;
    }
    if (line[73] != '/') {
      continue;
    }
    
    if (line[28]  == 'x' && line.find(program_name) != std::string::npos) {
      std::string addr_str = line.substr(0, 11);
      ret = stol(addr_str, 0, 16);
      //std::cout << ret << '\n';
      //std::cout << addr_str << '\n';
      break;
    } 
  }
  return ret; 
}


int main(int argc, char *argv[]) {
  const std::string target = TARGET_PROGRAM;
  const std::string corpus_dir = CORPUS_DIR;
  const std::string cases_dir = TEST_CASES_PATH;
  std::vector<corpus_file> corpus_files; 
  
  Tracer tracer = Tracer(target.c_str());
  Mutator mutator = Mutator(corpus_dir, SEED);

  while (1) {
    corpus_file test_case = mutator.get();

    bool found_crash = tracer.run(test_case.filename.c_str());

    if (found_crash) {
      std::cout << "[*] Found crash ...\n";
      std::cout << "content ";

      for (auto c: test_case.content) {
        std::cout << std::hex << int((uint8_t)c) << " ";
      }

      std::cout << "\n";
    }

  }

  return 0;
}
