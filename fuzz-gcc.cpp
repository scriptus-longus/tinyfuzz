#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <cstring>
#include "fuzz.hpp"
#include "utils.hpp"
#include "fuzz-gcc.hpp"
#include <stdint.h>

bool inserted_init = false;
bool in_main = false;

int main(int argc, char **argv) {
  if (argc < 3) { 
    std::cout << "no file to instrumentalize and/or no ouput file\n";
    return -1;
  }

  char *filename = argv[1];
  int nth_jump_instr = 0;

  std::ifstream file_stream(filename);
  std::string current_line;

  std::string outfile_content;

  while(std::getline(file_stream, current_line)) {
    outfile_content.append(current_line);
    outfile_content.append("\n");

    if (current_line == "\t.text" && !inserted_init) {
      // insert code to for function to set up shared memory
      outfile_content.append(get_shmem_init_code());

      inserted_init = true;
    } else if (current_line == "main:") {
      in_main = true;
    } else if (in_main == true && current_line == "\t.cfi_startproc") {
      in_main = false;
      outfile_content.append(CALL_SHMEM_INIT_CODE); 
    } else if (string_startswith(current_line, "\tj")) {
      if (nth_jump_instr < SHARED_MEM_SIZE) {
        outfile_content.append(get_jump_code(nth_jump_instr));
        nth_jump_instr++;
      } else {
        std::cout << "Not enough shared memory to map all jumps;\nYou might want to change the size of shared memory.\nAbort";
        return -1;
      }
    } else {
      continue;
    }
  }

  std::stringstream outfile_name;
  outfile_name << argv[2]; 

  dump_file(outfile_name.str(), outfile_content); 
  return 0;
}
