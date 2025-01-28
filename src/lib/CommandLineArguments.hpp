#pragma once
#include <string>


namespace sandbox {

/*
** This structure holds the input arguments
** given to the sandbox
*/
struct CommandLineArguments {
  std::string exec_name;
  std::string exec_args;
  std::string input_file;
  std::string output_file;
  std::string log_file;

  CommandLineArguments();

  // parse the arguments passed to the main executable
  void parse(int argc, const char *const argv[]);

  // handy wrapper for testing
  void parse(std::string const &input);

  void clear();

 private:
  void check();
};



}  // end namespace sandbox
