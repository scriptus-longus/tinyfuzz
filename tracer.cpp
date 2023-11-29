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

uint32_t INIT_ADDR = 0x1000;

class Tracer {
private:
  //std::string program;
  const char* program;
  std::vector<uint64_t> unique_crashes; 
  std::vector<Breakpoint> breakpoints;
  std::string elf_file;

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
  }

  CrashType run(const char *data) {
    int status;
    struct user_regs_struct regs;
    siginfo_t signal;
    std::vector<uint64_t> execution_path;
    pid_t proc;
  
    CrashType ret;
  
    proc = fork();
    if (proc == 0) {
      personality(ADDR_NO_RANDOMIZE);
      ptrace(PTRACE_TRACEME, proc, NULL, NULL);

      freopen("/dev/null", "w", stdout); // no stdout

      execl(program, program, data, NULL);
    } else {
      wait(NULL);

      for (auto& bp: this->breakpoints) { 
        ptrace(PTRACE_POKEDATA, proc, 
                                (uint64_t)(this->base + bp.addr), 
                                ((bp.instr & ~0xff) | (uint64_t)0xcc & 0xff));
      }

      ptrace(PTRACE_CONT, proc, NULL, NULL);

      while(waitpid(proc, &status, 0)) { 
        // check if program has terminated
        if (WIFEXITED(status)) {
          ptrace(PTRACE_GETSIGINFO, proc, NULL, &signal);
          ptrace(PTRACE_GETREGSET, proc, NULL, &regs);
          break;
        }

        ptrace(PTRACE_GETSIGINFO, proc, NULL, &signal);
        if (ptrace(PTRACE_GETREGS, proc, NULL, &regs) == -1) 
          std::cout << "cant read regs\n";

        if (signal.si_signo == SIGSEGV) {
          break;
        } else if (signal.si_signo == SIGTRAP) {
          execution_path.push_back(regs.rip);  // TODO: check independence
        }
      
        ptrace(PTRACE_CONT, proc, NULL, NULL);
      }

      ret.addr = regs.rip;
      ret.execution_path = execution_path;
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

