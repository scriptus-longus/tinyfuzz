#include <iostream>
#include <vector>
#include <string>
#include "tracer.cpp"
#include "mutator.cpp"
#include "fuzz.hpp"
#include <signal.h>
#include <stdint.h>

bool keep_fuzzing = true;

std::string cat_path(std::string path, std::string filename) {
  if (path.back() != '/') {
    path.append("/");
  }
  
  path.append(filename);
  return path;
}

void exit_signal_handler(int s) {
  std::cout << "exiting..." << '\n';
  keep_fuzzing = false;
}

int main(int argc, char *argv[]) {
  const std::string target = TARGET_PROGRAM;
  const std::string corpus_dir = CORPUS_DIR;
  const std::string cases_dir = TEST_CASES_PATH;
  std::vector<corpus_file> corpus_files; 

  signal(SIGINT, exit_signal_handler);
  
  Tracer tracer = Tracer(target.c_str());
  Mutator mutator = Mutator(corpus_dir, SEED);

   
  while (keep_fuzzing) {
    corpus_file test_case = mutator.get();

    ExecTrace trace = tracer.run(test_case.filename.c_str());

    trace.content = test_case.content;
 
    mutator.add(trace);
  }

  return 0;
}
