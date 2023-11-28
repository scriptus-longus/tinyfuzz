#include <iostream>
#include <string>
#include <dirent.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <tuple>

const uint8_t FLIP_ARRAY[] =  {1, 2, 4, 8, 16, 32, 64};
const int CYCLE_LENS[] =  {1, 2, 4, 8, 16, 32};
const int BLOCK_SIZES[] = {1, 2, 4, 8, 16, 32, 64};

typedef void (*mutation_func) (std::string &data, uint32_t idx);

struct corpus_file {
  std::string filename;
  std::string content;
} typedef corpus_file;

class Mutator {
private:
  std::string corpus_dir;
  std::string cases_path = "cases/";
  std::vector<corpus_file> pool;

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

  std::string read_content_corpus_file(std::string path) {
    std::ifstream file_stream(path);
    std::stringstream buffer;
    std::string ret;// = new std::string;


    buffer << file_stream.rdbuf();
    ret.append(buffer.str());

    file_stream.close();
    return ret;
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
                                read_content_corpus_file(this->corpus_dir + "/" + corpus_file_entry->d_name)}); 
      }
    }
    for (auto x: this->corpus) {
      std::cout << x.filename << " " << x.content << '\n';
    }
  }

  std::string mutate(std::string data){
    uint32_t n = CYCLE_LENS[rand() % 6];
    std::string ret = data; 

    //auto& mutations[] = {this->bit_flip, this->byte_flip, this->add_block, this->remove_block};

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

  void create_pool() {
    for (auto x: this->corpus) {
      this->pool.push_back({this->cases_path + "sample1.txt", mutate(x.content)}); 
    }

    /*
    for (auto x: this->pool) {
      std::cout << x.filename << " ";
      for (auto c: x.content) {
        std::cout << std::hex << (int)((uint8_t)c) << " ";
      }
      std::cout << "\n";
    }*/
  }


public:
  std::vector<corpus_file> corpus;

  Mutator (std::string corpus_dir, int seed) {
    srand(seed);
    this->corpus_dir = corpus_dir;
    this->load_corpus();
    this->create_pool();
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
  
    // TODO: put in create pool
    std::ofstream mutated_file(test_case.filename);
    mutated_file << test_case.content;
    mutated_file.close();

    std::string content = test_case.content;
    std::string filename = test_case.filename; 

    this->pool.pop_back();

    return {filename, content};
  }
};
