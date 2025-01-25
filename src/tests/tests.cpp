#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include "CommandLineArguments.hpp"
#include "Sandbox.hpp"

bool test_input() {
  std::string arg_str = "dummy_exec input_file.txt output_file.txt log.txt sandboxed_exec arg1 arg2 ";
  sandbox::CommandLineArguments args;
  args.parse(arg_str);
  if (args.exec_name != "sandboxed_exec")
    return false;
  if (args.input_file != "input_file.txt")
    return false;
  if (args.output_file != "output_file.txt")
    return false;
  if (args.log_file != "log.txt")
    return false;
  if (args.exec_args != "arg1 arg2")
  {
    std::cerr << args.exec_args << std::endl;
    return false;
  }
  return true;
}

bool test_wrong_input()
{
  std::string arg_str = "dummy_exec sandboxed_exec arg1 arg2 ";
  sandbox::CommandLineArguments args;
  try {
    args.parse(arg_str);
  }
  catch (std::invalid_argument & e) {
    return true;
  }
  return false;
}

bool test_python() {
  // std::string arg_str = "dummy_exec input_file.txt output_file.txt log.txt /usr/bin/python3 arg1 arg2 ";
  std::string arg_str = "dummy_exec input_file.txt output_file.txt log.txt abra arg1 arg2 ";
  sandbox::CommandLineArguments args;
  args.parse(arg_str);

  {
    std::ofstream out(args.input_file);
  }

  sandbox::Sandbox sandbox(args);
  sandbox.launch();

  std::filesystem::remove(args.input_file);

  return true;
}

template <typename F>
void run_test(F f, std::string const & name) {
  if (!f()) {
    std::cout << std::setw(20) << std::left << name << " [fail ❌]" << std::endl;
  }
  else {
    std::cout << std::setw(20) <<std::left << name << " [pass ✅]" << std::endl;
  }
}


auto main(int argc, char *argv[]) -> int {
  run_test(test_input, "input");
  run_test(test_wrong_input, "wrong input");
  run_test(test_python, "python");

  return 0;
}
