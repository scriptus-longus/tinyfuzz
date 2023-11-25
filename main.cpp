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
//using namespace std;

// parameters
#define TARGET_PROGRAM "target/simple"
#define CORPUS_DIR "corpus/"
#define TEST_CASES_PATH "cases/"

#define SEED 1337

const int FLIP_ARRAY[] =  {1, 2, 4, 8, 16, 32, 64, 128};

struct corpus_file {
  std::string filename;
  std::string content;
} typedef corpus_file;

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
  uint32_t bit = FLIP_ARRAY[rand() % 8];
  data[idx] = data[idx] ^ bit; 
}

std::string mutate_data(std::string data) {
  uint32_t idx = (uint32_t)(rand() % data.size());
  std::string ret = data; 
 
  bit_flip(ret, idx);
  return ret;
}

bool execute_fuzz(const char *program, const char *data) {
  pid_t proc;
  int status;
  siginfo_t targetsig;
  struct user_regs_struct regs;

  proc = fork();
  if (proc == 0) {
    ptrace(PTRACE_TRACEME, proc, NULL, NULL);
    freopen("/dev/null", "w", stdout); // no stdout
    execl(program, program, data, NULL);
  } else {
    int status;

    while(waitpid(proc, &status, 0) && ! WIFEXITED(status)) {
      struct user_regs_struct regs; 
      ptrace(PTRACE_GETSIGINFO, proc, NULL, &targetsig);
      ptrace(PTRACE_GETREGS, proc, NULL, &regs);

      
      ptrace(PTRACE_CONT, proc, NULL, NULL);
    }

    if (targetsig.si_signo == SIGSEGV) {
      return true;
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
  while (entry = readdir(dp)) {
    if (entry->d_name[0] != '.') {
      current_file.filename = entry->d_name; 
      current_file.content = read_file(cat_path(corpus_dir, entry->d_name));     
 
      corpus_files.push_back(current_file);
    }
  } 
  std::string tmp = cat_path(cases_dir, "sample1.txt");  // write to variable first so it does not get destroyed after ret
  const char *data = tmp.c_str();
  const char *program = target.c_str();

  // fuzzing loop
  while (1) {
    for (uint32_t i = 0; i < corpus_files.size(); i++) {
      bool found_crash = false;
      // write mutated data to file
      std::ofstream mutated_file(cat_path(cases_dir, "sample1.txt"));

      std::string content = mutate_data(corpus_files[0].content);
  
      mutated_file << content;

      mutated_file.close(); 
      // run the fuzzer
      found_crash = execute_fuzz(program, data);

      if (found_crash) {
        std::cout << "[*] Found crash ...\n";
        std::cout << "Content " << content;
      }
    }
  }
 
  return 0;
}
