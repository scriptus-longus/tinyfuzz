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
//using namespace std;

// parameters
#define TARGET_PROGRAM "target/simple"
#define CORPUS_DIR "corpus/"
#define TEST_CASES_PATH "cases/"
#define DEBUG_LOG true
#define BREAKPOINT_FILE ".bp_list"

#define SEED 1337

const int FLIP_ARRAY[] =  {1, 2, 4, 8, 16, 32, 64};
const int CYCLE_LENS[] =  {1, 2, 4, 8, 16, 32};

int INIT_ADDR = 0x1000;


struct corpus_file {
  std::string filename;
  std::string content;
} typedef corpus_file;

struct Breakpoint {
  int addr;
  uint64_t instr;
};


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
  uint32_t bit = FLIP_ARRAY[rand() % 7];
  data[idx] = data[idx] ^ bit; 
}

std::string mutate_data(std::string data) {
  uint32_t n = CYCLE_LENS[rand() % 6];
  std::string ret = data; 

  for (uint32_t i = 0; i < n; i++) {
    uint32_t idx = (uint32_t)(rand() % data.size());
 
    bit_flip(ret, idx);

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

void load_breakpoints(std::string bp_file_name) {
  // read breakpoint file and elf file to extract instruction
  std::ifstream     bp_file_stream (bp_file_name); 
  std::ifstream     elf            (TARGET_PROGRAM, std::fstream::binary); // TODO: change
  std::stringstream content;

  content << elf.rdbuf();

  std::string line;

  uint64_t instr;
  int idx;
  struct Breakpoint current_breakpoint;

  while (std::getline(bp_file_stream, line)) {
    idx = stoi(line.substr(0, 4), 0, 16);

    instr = 0;
    for (int i = 0; i < 8; i++) {
      instr += (((uint64_t)content.str()[idx+i] & 0xff) << i*8);
    }

    //std::cout << std::hex << instr <<std::endl;
 
    current_breakpoint = {.addr = idx - INIT_ADDR, .instr = instr}; 

    breakpoints.push_back(current_breakpoint);
  }

  /*
  for (int i = 0; i < breakpoints.size(); i++) {
    std::cout << std::hex << breakpoints[i].instr << std::endl;
  }*/
 
  return;
}
  

bool execute_fuzz(const char *program, const char *data) {
  pid_t proc;
  int status;
  siginfo_t targetsig;
  struct user_regs_struct regs;
  uint64_t base = 0x555555555000;
  //long main_addr = 0x5555555552c1;
  uint64_t addr;
  long breakpoint_instr;
  uint64_t int3 = 0x0;
  char instr = 0xcc;
  int3 = int3 + instr;
  std::vector<uint64_t> breakpoints_hit;

  proc = fork();
  if (proc == 0) {
    personality(ADDR_NO_RANDOMIZE);
    ptrace(PTRACE_TRACEME, proc, NULL, NULL);
    freopen("/dev/null", "w", stdout); // no stdout
    execl(program, program, data, NULL);
  } else {
    int status;
    wait(NULL);
    for (int i =0; i < breakpoints.size(); i++) { 
      //breakpoint_instr = ptrace(PTRACE_PEEKDATA, proc, main_addr, NULL);
      addr =  base + breakpoints[i].addr;
      ptrace(PTRACE_POKEDATA, proc, addr, ((breakpoints[i].instr & ~0xff) | (uint64_t)int3 & 0xff));
      //ptrace(PTRACE_POKEDATA, proc, main_addr, ((breakpoints_instr & ~0xff) | (uint64_t)int3 & 0xff));
      //breakpoint_instr = ptrace(PTRACE_PEEKDATA, proc, main_addr, NULL);
      //std::cout << std::hex << addr << std::endl;
    }
    ptrace(PTRACE_CONT, proc, NULL, NULL);

    while(waitpid(proc, &status, 0)) { 
      if (WIFEXITED(status)) {
        ptrace(PTRACE_GETSIGINFO, proc, NULL, &targetsig);
        ptrace(PTRACE_GETREGSET, proc, NULL, &regs);
        break;
      }

      ptrace(PTRACE_GETSIGINFO, proc, NULL, &targetsig);
      if (ptrace(PTRACE_GETREGS, proc, NULL, &regs) == -1) {
        std::cout << "cant read regs\n";
      }

      if (targetsig.si_signo == SIGSEGV) {
        break;
      } else if (targetsig.si_signo == SIGTRAP) {
        //std::cout << "hit breakpoint at " <<  std::hex <<regs.rip << '\n';
        breakpoints_hit.push_back(regs.rip);  // TODO: check independence
      }
      
      ptrace(PTRACE_CONT, proc, NULL, NULL);
    }

    // check if crash was sigsegv and unique
    if (targetsig.si_signo == SIGSEGV) {
      if (!std::count(known_addresses.begin(), known_addresses.end(), regs.rip)) {
        //std::cout << std::hex << regs.rip << '\n';
        known_addresses.push_back(regs.rip);
        return true;
      }
    }
  }
  return false;
}

int main(int argc, char *argv[]) {
  srand(SEED);
 
  const std::string target = TARGET_PROGRAM;
  const std::string corpus_dir = CORPUS_DIR;
  const std::string cases_dir = TEST_CASES_PATH;
  std::vector<corpus_file> corpus_files; 
   
  DIR *dp;
  struct dirent *entry;
  dp = opendir(corpus_dir.c_str());
  
  if (dp == NULL) {
    perror("No Corpus dir.");
    exit(0);
  }

  corpus_file current_file;
  // load all files and contents into corpus_file vecotr
  while ((entry = readdir(dp))) {
    if (entry->d_name[0] != '.') {
      current_file.filename = entry->d_name; 
      current_file.content = read_file(cat_path(corpus_dir, entry->d_name));     
 
      corpus_files.push_back(current_file);
    }
  } 

  load_breakpoints("bp_list");
  //return 0;

  std::string tmp = cat_path(cases_dir, "sample1.txt");  // write to variable first so it does not get destroyed after ret
  const char *data = tmp.c_str();
  const char *program = target.c_str();
  uint32_t content_size = 0;

  // fuzzing loop
  while (1) {
    for (uint32_t i = 0; i < corpus_files.size(); i++) {
      bool found_crash = false;
      // write mutated data to file
      std::ofstream mutated_file(cat_path(cases_dir, "sample1.txt"));

      std::string content = mutate_data(corpus_files[0].content);

      mutated_file << content;
      content_size = content.size();
      mutated_file.close(); 
      // run the fuzzer
      found_crash = execute_fuzz(program, data);

      if (found_crash) {
        std::cout << "[*] Found crash ...\n";
        std::cout << "content ";

        for (int i  = 0; i < content.size(); i++) {
          std::cout << int(content[i]) << " ";
          //std::cout << std::hex << (uint8_t)content[i] << " "; 
        }

        std::cout << "\n";
        if (DEBUG_LOG) {
          std::cout << "[DEBUG] Known crash addresses:\n";

          for (int i = 0; i < known_addresses.size(); i++) {
            std::cout << (void *)known_addresses[i] << " "; 
          } 
          std::cout << "\n";
        }
      }
    }
  }
 
  return 0;
}
