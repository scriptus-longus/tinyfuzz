#include <iostream>
#include <vector>
#include <string>
#include "tracer.cpp"
#include "mutator.cpp"
#include "fuzz.hpp"

// Tracer
//TODO: find base
//TODO: find offset
//TODO: implement proper instrumentation


// Mutator
//TODO: create actual pool with multiple mutations

std::string cat_path(std::string path, std::string filename) {
  if (path.back() != '/') {
    path.append("/");
  }
  
  path.append(filename);
  return path;
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

    CrashType crash = tracer.run(test_case.filename.c_str());

    crash.content = test_case.content;
    
    mutator.add(crash);

    if (crash.segfault) {
      std::cout << "[*] Found crash ... at 0x" << std::hex << crash.addr<< "\n";
      std::cout << "content ";

      for (auto c: crash.content) {
        std::cout << std::hex << int((uint8_t)c) << " ";
      }

      std::cout << "\n";
    }

  }

  return 0;
}
