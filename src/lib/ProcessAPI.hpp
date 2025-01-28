#pragma once
#include <array>
#include <string>

/*
** This header contains the API of the child and parent process functions
*/

namespace sandbox {

// hold the arguments passed to the parent process
struct ParentArgs {
  std::array<int, 2> pipeIn;
  std::array<int, 2> pipeOut;
  std::array<int, 2> pipeErr;
  int childId{0};
  std::string input_file;
  std::string output_file;
  std::string log_file;
};

// hold the arguments passed to the child process
struct ChildArgs {
  std::array<int, 2> pipeIn;
  std::array<int, 2> pipeOut;
  std::array<int, 2> pipeErr;
  std::string exec_name;
  std::string exec_args;
};

// implements the child process logic
auto child_process(ChildArgs & args) -> int;

// implements the parent process logic
auto parent_process(ParentArgs & args) -> int;


}  // end namespace sandbox
