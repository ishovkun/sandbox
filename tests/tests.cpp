#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sys/wait.h>
#include <sys/resource.h>
#include <thread>  // sleep
#include "CommandLineArguments.hpp"
#include "Sandbox.hpp"
#include "ProcessLauncher.hpp"
#include "PipeMonitor.hpp"
#include "App.hpp"
#include "writers.hpp"


struct test_args {
  std::array<int,2> pipeIn{-1, -1};
  std::array<int,2> pipeOut{-1, -1};
  std::array<int,2> pipeErr{-1, -1};
  int childId{0};
};

template <typename F>
void run_test(F f, std::string const & name) {
  if (!f()) {
    std::cout << std::setw(20) << std::left << name << " [fail ❌]" << std::endl;
  }
  else {
    std::cout << std::setw(20) <<std::left << name << " [pass ✅]" << std::endl;
  }
}


void closePipes(test_args & arg) {
  close(arg.pipeIn[0]);
  close(arg.pipeIn[1]);
  close(arg.pipeOut[0]);
  close(arg.pipeOut[1]);
  close(arg.pipeErr[0]);
  close(arg.pipeErr[1]);
}

void routeStdOut(test_args & arg) {
  dup2(arg.pipeOut[1], STDOUT_FILENO);
}

void routeStdErr(test_args & arg) {
  dup2(arg.pipeErr[1], STDERR_FILENO);
}

void closeWriter(test_args & arg) {
  close(arg.pipeIn[0]);
  close(arg.pipeOut[1]);
  close(arg.pipeErr[1]);
}

int waitPID0(int pid) {
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
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

bool test_hello() {
  std::string arg_str = "dummy input_file.txt output_file.txt log.txt "  SAMPLES_DIR  "/hello";
  sandbox::CommandLineArguments args;
  args.parse(arg_str);
  {
    std::ofstream out(args.input_file);
    out << "Sandbox test" << std::endl;
  }

  sandbox::App app(args);
  auto ret = app.run();

  namespace fs = std::filesystem;
  fs::remove(args.input_file);
  if (!fs::exists(args.output_file) || !fs::exists(args.log_file)) {
    std::cout << "output exists " << fs::exists(args.output_file) << std::endl;
    std::cout << "log exists " << fs::exists(args.log_file) << std::endl;
    return false;
  }

  if (fs::is_empty(args.output_file)) {
    std::cout << "output empty " << fs::is_empty(args.output_file) << std::endl;
    return false;
  }

  fs::remove(args.log_file);
  fs::remove(args.output_file);

  return ret == 0;
}

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
    if (sandbox.setup() != 0)
      throw std::runtime_error("sandbox setup failed");

    namespace fs = std::filesystem;
    int cnt{0};
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
      std::cout << "entry " << entry << std::endl;
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

    {
      std::ofstream out("should_be_deleted.txt");
      out << "this will disappear " << std::endl;
    }


    std::exit(EXIT_SUCCESS);
  };
  auto parentFunc = [](test_args & arg) -> int {
    closePipes(arg);
    return waitPID0(arg.childId);
  };
  sandbox::ProcessLauncher<test_args, test_args> launcher;
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
    int written{0}, toWrite{3};
    while (elapsed < std::chrono::duration<double>(0.01)) {
      elapsed = std::chrono::high_resolution_clock::now() - begin;
      if (++written < toWrite)
        std::cout << "heartbeat" << std::endl;
    }
    return EXIT_SUCCESS;
  };

  auto parentFunc = [](test_args & arg) -> int {
    closeWriter(arg);
    sandbox::PipeMonitor monitor(arg.childId);
    // check that pipe is added
    std::ostringstream stdout;
    if (0 != monitor.addPipe(arg.pipeOut[0], stdout, sandbox::add_timestamp))
      return false;

    monitor.start();

    auto countLines = [](auto const & s) {
     return std::count(s.begin(), s.end(), '\n') + 1;
    };
    // child should yield 2 lines
    // resolution should be sufficient enough to have different timestamps
    auto output{stdout.str()};
    if (countLines(output) != 2)
      return false;
    std::string line1, line2;
    std::istringstream iss(output);
    std::getline(iss, line1);
    std::getline(iss, line2);
    if (line1 == line2) return false;

    auto ret = waitPID0(arg.childId);
    closePipes(arg);
    return ret;
  };
  sandbox::ProcessLauncher<test_args, test_args> launcher;
  auto ret = launcher.run(parentFunc, parentArgs, childFunc, childArgs);
  return ret == 0;
}

bool test_resources() {
  std::string arg_str = "dummy input_file.txt output_file.txt log.txt "  SAMPLES_DIR  "/beast";
  sandbox::CommandLineArguments args;
  args.parse(arg_str);
  {
    std::ofstream out(args.input_file);
    out << "Sandbox test" << std::endl;
  }

  sandbox::App app(args);
  auto ret = app.run();

  namespace fs = std::filesystem;
  fs::remove(args.input_file);
  std::ifstream in(args.log_file);

  if (fs::is_empty(args.output_file))
    return false;

  fs::remove(args.log_file);
  fs::remove(args.output_file);

  struct rusage usage;
  getrusage(RUSAGE_CHILDREN | RUSAGE_SELF, &usage);

  if (usage.ru_maxrss < 250'000)
    return false;

  return ret == 0;
}

auto test_timestamp_writer() -> bool {
  std::string message = "I love coding";
  std::ostringstream out1, out2;
  sandbox::add_timestamp(out1, message.data(), message.size());
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  sandbox::add_timestamp(out2, message.data(), message.size());
  return out1.str() != out2.str();
}

auto test_comma_replacement() -> bool {
  std::string message = "This,message,has,4,commas";
  auto n_commas1 = std::count_if(message.begin(), message.end(), [](char c) { return c == ','; });
  std::ostringstream out1, out2;
  sandbox::replace_commas_with_tabs(out1, message.data(), message.size());
  auto n_commas2 = std::count_if(message.begin(), message.end(), [](char c) { return c == ','; });
  auto n_tabs2 = std::count_if(message.begin(), message.end(), [](char c) { return c == '\t'; });
  return n_commas2 == 0 && n_commas1 == n_tabs2;
}

auto main(int argc, char *argv[]) -> int {
  run_test(test_input, "input");
  run_test(test_wrong_input, "wrong input");
  run_test(test_launcher, "launcher");
  run_test(test_sandbox, "sandboxing");
  run_test(test_timestamp_writer, "timestamp writer");
  run_test(test_comma_replacement, "replace commas");
  run_test(test_pipe_monitor, "pipe monitor");
  run_test(test_hello, "hello");
  run_test(test_resources, "resources");
  return 0;
}
