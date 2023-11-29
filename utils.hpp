#ifndef HELPER_FUNCTIONS
#define HELPER_FUNCTIONS
#include <string>
#include <fstream>
#include <sstream>
#include <string>

std::string read_file(std::string path) {
  std::ifstream file_stream(path);
  std::stringstream buffer;
  std::string ret;


  buffer << file_stream.rdbuf();
  ret = buffer.str();

  file_stream.close();
  return ret;
}

std::string int_to_str(int x) {
  std::stringstream _stream;
  _stream << x;
  
  return _stream.str();

}

std::string long_to_str(long x) {
  std::stringstream _stream;
  _stream << x;
  
  return _stream.str();
}

void printhex(std::string s) {
  for (auto c: s) {
    std::cout << std::hex << (int)((uint8_t)c) << " ";
  }
}

void printhexnl(std::string s) {
  printhex(s);
  std::cout << '\n';
}
#endif 
