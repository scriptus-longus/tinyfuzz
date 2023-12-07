#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <cstring>
#include <sys/personality.h>
#include <sys/shm.h>
#include <stdint.h>
#include "fuzz.hpp"

class Tracer {
private:
  const char* program;
  int shared_mem_segid;
  int *shared_mem_addr;

public:
  Tracer(const char* program) {
    this->program = program;
    
    this->shared_mem_segid = shmget(0x1001, SHARED_MEM_SIZE*sizeof(int), 0x780);
  }
  
  ~Tracer() {
    shmctl(this->shared_mem_segid, IPC_RMID, 0);
  }

  ExecTrace run(const char *data) {
    int status;
    struct user_regs_struct regs;
    siginfo_t signal;
    pid_t proc;
  
    ExecTrace ret;
  
    proc = fork();
    if (proc == 0) {
      personality(ADDR_NO_RANDOMIZE);
      ptrace(PTRACE_TRACEME, proc, NULL, NULL);

      freopen("/dev/null", "w", stdout); // no stdout

      execl(this->program, this->program, data, NULL);     // TODO: fuzzing without execl
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
      this->shared_mem_addr = (int *)shmat(this->shared_mem_segid, 0, 0);

      memcpy(ret.execution_trace, this->shared_mem_addr, SHARED_MEM_SIZE*sizeof(int)); // TODO: check size

      memset(this->shared_mem_addr, 0, SHARED_MEM_SIZE*sizeof(int));
      shmdt((void *)this->shared_mem_addr);

      ret.addr = regs.rip;
      ret.segfault = signal.si_signo == SIGSEGV;      
    }
    return ret;

  }
};

