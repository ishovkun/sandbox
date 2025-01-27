#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sys/wait.h>
#include "CommandLineArguments.hpp"
#include "Sandbox.hpp"
#include "ProcessLauncher.hpp"
#include "PipeMonitor.hpp"


struct test_args {
  std::array<int,2> pipeIn{-1, -1};
  std::array<int,2> pipeOut{-1, -1};
  int childId{0};
};

void closePipes(test_args & arg) {
  close(arg.pipeIn[0]);
  close(arg.pipeIn[1]);
  close(arg.pipeOut[0]);
  close(arg.pipeOut[1]);
}

void routeStdOut(test_args & arg) {
  dup2(arg.pipeOut[1], STDOUT_FILENO);
}

void closeWriter(test_args & arg) {
  close(arg.pipeIn[0]);
  close(arg.pipeOut[1]);
}

int waitPID0(int pid) {
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    // std::cout << "Process exited with status " << WEXITSTATUS(status) << "\n";
  } else if (WIFSIGNALED(status)) {
    std::cout << "Process killed by signal " << WTERMSIG(status) << "\n";
  }
  return status;
}

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

// bool test_python() {
//   std::string arg_str = "dummy_exec input_file.txt output_file.txt log.txt /usr/bin/python3 arg1 arg2 ";
//   sandbox::CommandLineArguments args;
//   args.parse(arg_str);

//   {
//     std::ofstream out(args.input_file);
//   }

//   sandbox::Sandbox sandbox(args);
//   sandbox.launch();

//   std::filesystem::remove(args.input_file);

//   return true;
// }

// bool test_hello() {
//   std::string arg_str = "dummy input_file.txt output_file.txt log.txt "  SAMPLES_DIR  "/hello";
//   sandbox::CommandLineArguments args;
//   args.parse(arg_str);

//   {
//     std::ofstream out(args.input_file);
//   }

//   sandbox::Sandbox sandbox(args);
//   sandbox.launch();

//   std::filesystem::remove(args.input_file);

//   return true;
// }

bool test_launcher() {
  test_args parentArgs, childArgs;
  auto childFunc = [](test_args & arg) -> void {
    closePipes(arg);
    exit(0);
  };
  auto parentFunc = [](test_args & arg) -> int {
    closePipes(arg);
    return 0;
  };
  sandbox::ProcessLauncher<test_args, test_args> launcher;
  launcher.run(parentFunc, parentArgs, childFunc, childArgs);
  return true;
}

bool test_sandbox() {
  test_args parentArgs, childArgs;
  auto childFunc = [](test_args & arg) -> int {
    auto curdir = std::filesystem::current_path();
    sandbox::Sandbox sandbox;
    sandbox.setup();

    namespace fs = std::filesystem;
    int cnt{0};
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
      if (entry.is_regular_file()) {
        cnt++;
      }
    }
    // check that current path is unavailable
    std::error_code ec;
    std::filesystem::current_path(curdir, ec);
    if (!ec) {
      std::exit(EXIT_FAILURE);
    }
    // check that samples dir is unavailable
    std::filesystem::current_path(std::string(SAMPLES_DIR), ec);
    if (!ec) {
      std::exit(EXIT_FAILURE);
    }
    std::exit(EXIT_SUCCESS);
  };
  auto parentFunc = [](test_args & arg) -> int {
    closePipes(arg);
    return waitPID0(arg.childId);
  };
  sandbox::ProcessLauncher<test_args, test_args> launcher;
  // child function should fail
  auto ret = launcher.run(parentFunc, parentArgs, childFunc, childArgs);
  return ret == 0;
}

bool test_pipe_monitor() {
  test_args parentArgs, childArgs;
  auto childFunc = [](test_args & arg) -> int {
    close(arg.pipeIn[1]);
    close(arg.pipeOut[0]);

    dup2(arg.pipeIn[0], STDIN_FILENO);
    dup2(arg.pipeOut[1], STDOUT_FILENO);
    // dup2(pipeOut[1], STDERR_FILENO);
    auto begin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed{0};
    while (elapsed < std::chrono::duration<double>(10000.5)) {
      elapsed = std::chrono::high_resolution_clock::now() - begin;
      std::cout << "beer" << std::endl;
    }
    return EXIT_SUCCESS;
  };
  auto parentFunc = [](test_args & arg) -> int {
    closeWriter(arg);
    sandbox::PipeMonitor monitor(arg.childId);
    // check that pipe is added
    if (0 != monitor.addPipe(arg.pipeOut[0], std::cout, true))
      return false;
    std::cout << "added pipe" << std::endl;
    monitor.start();

    // monitor.start();
    auto ret = waitPID0(arg.childId);
    closePipes(arg);
    return ret;
  };
  sandbox::ProcessLauncher<test_args, test_args> launcher;
  auto ret = launcher.run(parentFunc, parentArgs, childFunc, childArgs);
  return ret == 0;
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
  // run_test(test_wrong_input, "wrong input");
  // run_test(test_python, "python");
  // run_test(test_hello, "hello");
  run_test(test_launcher, "launcher");
  // run_test(test_isolated_launcher, "isolated launcher");
  run_test(test_sandbox, "sandboxing");
  run_test(test_pipe_monitor, "pipe monitor");

  return 0;
}
