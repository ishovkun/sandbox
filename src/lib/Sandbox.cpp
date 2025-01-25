#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "Sandbox.hpp"
#include "FileSystemManager.hpp"

namespace sandbox {


Sandbox::Sandbox(CommandLineArguments const & args)
    : _args(args)
{
  namespace fs = std::filesystem;
  // if (!fs::exists(args.exec_name)) {
  //   throw std::invalid_argument("Executable does not exist");
  // }
  if (!fs::exists(args.input_file)) {
    throw std::invalid_argument("Input file does not exist");
  }

  if (!std::filesystem::is_regular_file(args.input_file) ||
      (std::filesystem::status(args.input_file).permissions() & fs::perms::owner_read) == fs::perms::none) {
    throw std::invalid_argument("Input file is not executable");
  }
}

std::string read_input(std::string const & input_file) {
  std::ifstream iss(input_file);
  if (!iss.is_open()) {
    throw std::runtime_error("Could not open input file");
  }
  std::ostringstream buf;
  buf << iss.rdbuf();
  return buf.str();
}

void parent_process() {

}

void child_process(int pipeIn[2], int pipeOut[2], std::string const & input, std::string const & exec_name, std::string const & args) {
  close(pipeIn[1]);
  close(pipeOut[0]);

  dup2(pipeIn[0], STDIN_FILENO);
  dup2(pipeOut[1], STDOUT_FILENO);
  dup2(pipeOut[1], STDERR_FILENO);

  close(pipeIn[0]);
  close(pipeOut[1]);

  std::string exec = exec_name + " " + args;
  char * const argv[] = {exec.data(), nullptr};
  std::cout << "launching " << exec << std::endl;
  execvp(exec_name.data(), argv);
  throw std::runtime_error("Could not execute command");
}

void Sandbox::launch() {
  auto input = read_input(_args.input_file);

  int pipeIn[2], pipeOut[2];
  if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1) {
    throw std::runtime_error("Could not create pipes");
  }

  std::cout << "forking" << std::endl;
  pid_t pid = fork();
  if (pid == -1) {
    throw std::runtime_error("Could not fork");
  }

  if (pid == 0) {
    std::cout << "run child" << std::endl;
    child_process(pipeIn, pipeOut, input, _args.exec_name, _args.exec_args);
  }
  else {
    std::cout << "run parent" << std::endl;
    parent_process();
  }
}

}  // end namespace sandbox
