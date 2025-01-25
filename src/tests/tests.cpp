#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sys/wait.h>
#include "CommandLineArguments.hpp"
#include "Sandbox.hpp"
#include "ProcessLauncher.hpp"
#include "IsolatedProcessLauncher.hpp"

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
  std::string arg_str = "dummy_exec input_file.txt output_file.txt log.txt /usr/bin/python3 arg1 arg2 ";
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

bool test_hello() {
  std::string arg_str = "dummy input_file.txt output_file.txt log.txt "  SAMPLES_DIR  "/hello";
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

bool test_launcher() {
  struct test_args {
    std::array<int,2> pipeIn{-1, -1};
    std::array<int,2> pipeOut{-1, -1};
    int childId{0};
  };
  test_args parentArgs, childArgs;
  auto childFunc = [](test_args & arg) -> void {
    close(arg.pipeIn[0]);
    close(arg.pipeIn[1]);
    close(arg.pipeOut[0]);
    close(arg.pipeOut[1]);
    exit(0);
  };
  auto parentFunc = [](test_args & arg) -> int {
    close(arg.pipeIn[0]);
    close(arg.pipeIn[1]);
    close(arg.pipeOut[0]);
    close(arg.pipeOut[1]);
    return 0;
  };
  sandbox::ProcessLauncher<test_args, test_args> launcher;
  launcher.run(parentFunc, parentArgs, childFunc, childArgs);
  return true;
}

bool test_isolated_launcher() {
  struct test_args {
    std::array<int,2> pipeIn{-1, -1};
    std::array<int,2> pipeOut{-1, -1};
    int childId{0};
  };
  test_args parentArgs, childArgs;
  auto childFunc = [](test_args & arg) -> int {
    close(arg.pipeIn[0]);
    close(arg.pipeIn[1]);
    close(arg.pipeOut[0]);
    close(arg.pipeOut[1]);
    std::ifstream iss("/home/ishovkun/ishovkun.zsh-theme");
    // std::ifstream iss(__FILE__);
    if (!iss.is_open()) {
      std::cerr << "file not found" << std::endl;
      exit(1);
    }
    std::ostringstream buf;
    buf << iss.rdbuf();
    std::cout << buf.str() << std::endl;
    exit(0);
    return 0;
  };
  auto parentFunc = [](test_args & arg) -> int {
    close(arg.pipeIn[0]);
    close(arg.pipeIn[1]);
    close(arg.pipeOut[0]);
    close(arg.pipeOut[1]);
    int status;
    waitpid(arg.childId, &status, 0);
    if (WIFEXITED(status)) {
      std::cout << "Process exited with status " << WEXITSTATUS(status) << "\n";
    } else if (WIFSIGNALED(status)) {
      std::cout << "Process killed by signal " << WTERMSIG(status) << "\n";
    }
    return 0;
  };
  sandbox::IsolatedProcessLauncher<test_args, test_args> launcher;
  launcher.run(parentFunc, parentArgs, childFunc, childArgs);
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
  // run_test(test_input, "input");
  // run_test(test_wrong_input, "wrong input");
  // run_test(test_python, "python");
  // run_test(test_hello, "hello");
  // run_test(test_launcher, "launcher");
  run_test(test_isolated_launcher, "isolated launcher");

  return 0;
}
