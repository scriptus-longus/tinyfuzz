#include <iostream>
#include <vector>
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
#include "fuzz.hpp"
#include <fcntl.h>
#include <sys/mman.h>

uint32_t INIT_ADDR = 0x1000;

class Tracer {
private:
  //std::string program;
  const char* program;
  std::vector<uint64_t> unique_crashes; 
  std::vector<Breakpoint> breakpoints;
  std::string elf_file;
  int *trace_addr;

  const uint64_t base = 0x555555555000; // TODO: change to config

  void load_breakpoints(std::string bp_file_name) {
    std::ifstream     bp_file_stream (bp_file_name); 

    std::string line;

    uint64_t instr;
    uint32_t idx;

    while (std::getline(bp_file_stream, line)) {
      idx = stoi(line.substr(0, 4), 0, 16);

      instr = 0;
      // load 8 bytes
      for (int i = 0; i < 8; i++) {
        instr += (((uint64_t)this->elf_file[idx+i] & 0xff) << i*8);
      }

      this->breakpoints.push_back({idx - INIT_ADDR, instr});
    }

    return;
  }
  

public:
  Tracer(const char* program) {
    this->program = program;

    // read file and store to string so we can later dump instructions for breakpoint creation 
    std::ifstream     elf(this->program, std::fstream::binary); 
    std::stringstream content;
    content << elf.rdbuf();

    this->elf_file = content.str();

    load_breakpoints(BREAKPOINT_FILE); 

    int fd = shm_open(STORAGE_ID, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      perror("could not open shared memory");
      exit(0);
    }

    this->trace_addr = (int *)mmap(NULL, sizeof(int)*N_BRANCHES, PROT_READ, MAP_SHARED, fd, 0);
    if (this->trace_addr == MAP_FAILED) {
      perror("mmap"); 
      exit(0);
    }

  }

  CrashType run(const char *data) {
    int status;
    struct user_regs_struct regs;
    siginfo_t signal;
    //std::vector<uint64_t> execution_path;
    int execution_trace[N_BRANCHES];    
    pid_t proc;
  
    CrashType ret;
  
    proc = fork();
    if (proc == 0) {
      personality(ADDR_NO_RANDOMIZE);
      ptrace(PTRACE_TRACEME, proc, NULL, NULL);

      freopen("/dev/null", "w", stdout); // no stdout

      execl(program, program, data, NULL);
    } else {

      while(waitpid(proc, &status, 0)) { 
        // check if program has terminated
        if (WIFEXITED(status)) {
          ptrace(PTRACE_GETSIGINFO, proc, NULL, &signal);
          ptrace(PTRACE_GETREGSET, proc, NULL, &regs);
          break;
        }

        if (ptrace(PTRACE_GETSIGINFO, proc, NULL, &signal) == -1) 
          std::cout << "received signal, but can't get further information\n";
        if (ptrace(PTRACE_GETREGS, proc, NULL, &regs) == -1) 
          std::cout << "cant read regs\n";

        if (signal.si_signo == SIGSEGV) {
          break;
        }         
        ptrace(PTRACE_CONT, proc, NULL, NULL);
      }

      // program has exited
      memcpy(ret.execution_trace, this->trace_addr, sizeof(int)*N_BRANCHES);

      ret.addr = regs.rip;
      ret.segfault = false;

      // check if crash was sigsegv and unique
      if (signal.si_signo == SIGSEGV) {
        // filter unique crashes
        if (DEBUG_LOG) {
          std::cout << "==========UNIQUE CRASHES IN TRACER===========\n";
          for (auto x: this->unique_crashes) 
            std::cout << std::hex << x << "\n";
          std::cout << "=============================================\n"; 

        } 
          
        // TODO: remove from tracer 
        if (!std::count(this->unique_crashes.begin(), this->unique_crashes.end(), regs.rip)) {
          this->unique_crashes.push_back(regs.rip);
          //return true;
          ret.segfault = true;
        }
      }
    }
    return ret;

  }
};

