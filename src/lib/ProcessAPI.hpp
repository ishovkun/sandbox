#pragma once
#include <array>
#include <string>

namespace sandbox {

struct ParentArgs {
  std::array<int, 2> pipeIn;
  std::array<int, 2> pipeOut;
  std::array<int, 2> pipeErr;
  int childId{0};
  std::string input_file;
  std::string output_file;
  std::string log_file;
};

struct ChildArgs {
  std::array<int, 2> pipeIn;
  std::array<int, 2> pipeOut;
  std::array<int, 2> pipeErr;
  std::string exec_name;
  std::string exec_args;
};

auto child_process(ChildArgs & args) -> int;

auto parent_process(ParentArgs & args) -> int;


}  // end namespace sandbox
