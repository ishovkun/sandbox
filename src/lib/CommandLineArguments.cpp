#include <cstddef>
#include <memory>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include "CommandLineArguments.hpp"

namespace sandbox {

CommandLineArguments::CommandLineArguments() {
  clear();
}

void CommandLineArguments::clear() {
  input_file.clear();
  output_file.clear();
  log_file.clear();
  exec_name.clear();
  exec_args.clear();
}

void CommandLineArguments::parse(int argc, const char * const argv[]) {
  std::string input;
  input += argv[0];
  for (int i = 1; i < argc; i++) {
    std::string token(argv[i]);
    input += ' ' + token;
  }
  parse(input);
}

void CommandLineArguments::parse(std::string const &s)
{
  std::istringstream iss(s);
  std::string token;
  iss >> token;
  iss >> input_file;
  iss >> output_file;
  iss >> log_file;
  iss >> exec_name;
  while (iss >> token) {
    for (auto c : token) {
      exec_args.push_back(c);
    }
    exec_args.push_back(' ');
  }
  if (exec_args.size())
    exec_args.pop_back();
  check();
}

void CommandLineArguments::check() {
  if (exec_name.empty()) {
    throw std::invalid_argument("No executable name provided");
  }
  if (input_file.empty()) {
    throw std::invalid_argument("No input file provided");
  }
  if (output_file.empty()) {
    throw std::invalid_argument("No output file provided");
  }
  if (log_file.empty()) {
    throw std::invalid_argument("No log file provided");
  }
}


}  // end namespace sandbox
