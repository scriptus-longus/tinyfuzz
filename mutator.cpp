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
  std::vector<ExecTrace> observed_traces;            // not known crashes 

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

  bool trace_eq(ExecTrace c1, ExecTrace c2) {
    if (memcmp(c1.execution_trace, c2.execution_trace, sizeof(int)*SHARED_MEM_SIZE) == 0 && c1.addr == c2.addr) {
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
    for (int i = 0; i < this->observed_traces.size(); i++) {
      std::stringstream filename;
      filename << this->cases_path << "/" << "sample_" << long_to_str(this->observed_traces[i].id) << ".spl";

      std::string content = mutate(this->observed_traces[i].content);

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

  void add(ExecTrace trace) {
    for (int i = 0;  i < this->observed_traces.size(); i++) {
      if (trace_eq(this->observed_traces[i], trace)) {
        // check if content is smaller to truncate crash
        if (this->observed_traces[i].content.size() > trace.content.size() && 
            this->observed_traces[i].content.size() >= MIN_TRACE_CONTENT_SIZE) {
          trace.id = observed_traces[i].id;
          this->observed_traces[i] = trace; 

          //std::stringstream filename;
          //filename << this->crashes_path << "/" << "crash_" << long_to_str(crash.id) << ".out";
          //dump_file(filename.str(), crash.content);

          if (DEBUG_LOG) {
            std::cout << "+-----------------------------------------------------------------------------+\n";
            std::cout << "[*] found better representation " << "\n";
            std::cout << "[*] Crash at 0x" << std::hex << trace.addr << " with content: \n";
            printhexnl(trace.content); 
          

            std::cout << "[*] all crashes: \n";
            for (int j = 0;  j < this->observed_traces.size(); j++) {
              std::cout << "0x" << std::hex << this->observed_traces[j].addr << " " << this->observed_traces[j].content.size() << "\n";
            }
            std::cout << "+-----------------------------------------------------------------------------+\n";
          }
        }

        return;
      }
    }
   
    if (trace.segfault) {
      std::cout << "+-----------------------------------------------------------------------------+\n";
      std::cout << "[*] FOUND a unique crash of length" << trace.content.size() << "\n";
      std::cout << "[*] Crash at 0x" << std::hex << trace.addr << " with content: \n";
      printhexnl(trace.content); 
      std::cout << "+-----------------------------------------------------------------------------+\n";

      std::stringstream filename;
      filename << this->crashes_path << "/" << "crash_" << long_to_str(trace.id) << ".out";
      dump_file(filename.str(), trace.content);
    }

    trace.id = rand() % 0xffffff; 
    std::cout << trace.id << "\n";
    this->observed_traces.push_back(trace);

    if (DEBUG_LOG && !trace.segfault) {
      std::cout << "+-----------------------------------------------------------------------------+\n";
      std::cout << "[*] Found new Execution Trace of legth: " << trace.content.size() << "\n";
      std::cout << "[*] End at 0x" << std::hex << trace.addr << " with content: \n";
      printhexnl(trace.content); 
          

      std::cout << "[*] all observed traces: \n";
      for (int j = 0;  j < this->observed_traces.size(); j++) {
        std::cout << "id: " << this->observed_traces[j].id << " finished at " << "0x" << std::hex << this->observed_traces[j].addr << " " << this->observed_traces[j].content.size() << "\n";
      }
      std::cout << "+-----------------------------------------------------------------------------+\n";
    }

    //std::stringstream filename;
    //filename << this->crashes_path << "/" << "crash_" << long_to_str(crash.id) << ".out";
    //dump_file(filename.str(), crash.content);

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
