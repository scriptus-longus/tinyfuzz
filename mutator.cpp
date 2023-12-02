#include <iostream>
#include <string>
#include <dirent.h>
#include <vector>
#include <fstream>
#include <sstream>
#include "fuzz.hpp"
#include "utils.hpp"

class Mutator {
private:
  std::string corpus_dir;
  std::string cases_path = "cases/";  // TODO: config
  std::string crashes_path = CRASHES_PATH;
  std::vector<corpus_file> pool;
  std::vector<CrashType> known_crashes;

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

  void load_corpus() {
    DIR *corpus_dir_ptr;
    struct dirent *corpus_file_entry;

    corpus_dir_ptr = opendir(this->corpus_dir.c_str());

    if (corpus_dir_ptr == NULL) {
      perror("No corpus dir.");
      exit(0); 
    } 

    while((corpus_file_entry = readdir(corpus_dir_ptr))) {
      if (corpus_file_entry->d_name[0] != '.') {
        this->corpus.push_back({corpus_file_entry->d_name, 
                                read_file(this->corpus_dir + "/" + corpus_file_entry->d_name)}); 
      }
    }
  }

  std::string mutate(std::string data){
    uint32_t n = CYCLE_LENS[rand() % 6];
    std::string ret = data; 

    for (uint32_t i = 0; i < n; i++) {
      uint32_t idx = (uint32_t)(rand() % ret.size());
 
      switch(rand() % 4) {
        case 0: this->bit_flip(ret, idx); break; 
        case 1: this->byte_flip(ret, idx); break; 
        case 2: this->add_block(ret, idx); break; 
        case 3: this->remove_block(ret, idx); break; 
      }
    }
    return ret;
  }

  bool crash_eq(CrashType c1, CrashType c2) {
    if (memcmp(c1.execution_trace, c2.execution_trace, sizeof(int)*N_BRANCHES) == 0 && c1.addr == c2.addr) {
      return true;
    } 
    return false;
  }

  void create_pool() {
    // TODO: multiple mutions from one corpus sample 

    for (int i = 0; i < this->corpus.size(); i++) {
      std::stringstream filename;
      filename << this->cases_path << "/" << "core_sample_" << i << ".spl";
      std::string content = mutate(this->corpus[i].content);

      this->pool.push_back({filename.str(), content}); 
      dump_file(filename.str(), content);
    }

    // sample from crashes
    for (int i = 0; i < this->known_crashes.size(); i++) {
      std::stringstream filename;
      filename << this->cases_path << "/" << "sample_" << long_to_str(this->known_crashes[i].id) << ".spl";
      std::string content = mutate(this->known_crashes[i].content);

      this->pool.push_back({filename.str(), content});
      dump_file(filename.str(), content);
    }
  }


public:
  std::vector<corpus_file> corpus;

  Mutator (std::string corpus_dir, int seed) {
    srand(seed);
    this->corpus_dir = corpus_dir;
    this->load_corpus();
    this->create_pool();
  }

  void add(CrashType crash) {

    for (int i = 0;  i < this->known_crashes.size(); i++) {
      //if (this->known_crashes[i].execution_path == crash.execution_path){
      if (crash_eq(this->known_crashes[i], crash)) {
        // check if content is smaller to truncate crash
        if (this->known_crashes[i].content.size() > crash.content.size() && this->known_crashes[i].content.size() >= 10) {
          crash.id = known_crashes[i].id;
          this->known_crashes[i] = crash; 

          std::stringstream filename;
          filename << this->crashes_path << "/" << "crash_" << long_to_str(crash.id) << ".out";
          dump_file(filename.str(), crash.content);

          if (DEBUG_LOG) {
            std::cout << "+-----------------------------------------------------------------------------+\n";
            std::cout << "[*] found better representation " << this->known_crashes[i].content.size() << " " << crash.content.size() << "\n";
            std::cout << "[*] Crash at 0x" << std::hex << crash.addr << " with content: \n";
            printhexnl(crash.content); 
          

            std::cout << "[*] all crashes: \n";
            for (int j = 0;  j < this->known_crashes.size(); j++) {
              std::cout << "0x" << std::hex << this->known_crashes[j].addr << " " << this->known_crashes[j].content.size() << "\n";
            }
            std::cout << "+-----------------------------------------------------------------------------+\n";
          }
        }

        return;
      }
    }
   
    crash.id = rand() % 0xffffff; 
    std::cout << crash.id << "\n";
    this->known_crashes.push_back(crash);


    std::stringstream filename;
    filename << this->crashes_path << "/" << "crash_" << long_to_str(crash.id) << ".out";
    dump_file(filename.str(), crash.content);

    return; 
  }  

  corpus_file get() {
    // check if pool exists if not create
    // select from pool
    // pop from pool
    // return 
    if (this->pool.size() == 0) {
      this->create_pool();
    }
    corpus_file test_case  = this->pool.back();

    std::string content = test_case.content;
    std::string filename = test_case.filename;

    this->pool.pop_back();

    return {filename, content};
  }
};
