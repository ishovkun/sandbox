#pragma once
#include <string>

namespace sandbox {

/*
**
** ./sandboxed_exec input output logfile <executable> <executable args ...>
** where <executable> is any executable that is chosen by the user and
** <executable_args> are the arguments needed by the executable
*/
struct CommandLineArguments {
  std::string exec_name;
  std::string exec_args;
  std::string input_file;
  std::string output_file;
  std::string log_file;

  CommandLineArguments();

  void parse(int argc, const char *const argv[]);

  void parse(std::string const &input);

  void clear();
  private:
  void check();
};



}  // end namespace sandbox
