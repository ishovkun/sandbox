#include <filesystem>
#include <stdexcept>
// #include <fstream>
// #include <sstream>
// #include <iostream>

// #include <sys/types.h>
// #include <sys/wait.h>
// #include <sys/resource.h>

// #include <chrono>
// #include <ctime>

#include "App.hpp"
#include "ProcessAPI.hpp"
#include "ProcessLauncher.hpp"

namespace sandbox {


App::App(CommandLineArguments const & args)
    : _args(args)
{
  namespace fs = std::filesystem;
  if (!fs::exists(args.exec_name)) {
    throw std::invalid_argument("Executable does not exist " + args.exec_name);
  }
  if (!fs::exists(args.input_file)) {
    throw std::invalid_argument("Input file does not exist");
  }

  if (!std::filesystem::is_regular_file(args.input_file) ||
      (std::filesystem::status(args.input_file).permissions() & fs::perms::owner_read) == fs::perms::none) {
    throw std::invalid_argument("Input file is not executable");
  }
}

// std::string read_input(std::string const & input_file) {
//   std::ifstream iss(input_file);
//   if (!iss.is_open()) {
//     throw std::runtime_error("Could not open input file");
//   }
//   std::ostringstream buf;
//   buf << iss.rdbuf();
//   return buf.str();
// }

// void parent_process(int pipeIn[2], int pipeOut[2], int child_id, std::string const Read child's stdout and stderr
//   // auto & logger = logging::Logger::ref();
//   // logger.set_file(args.output_file);

//   // std::ofstream out(args.output_file);
//   // std::ofstream logStream(args.log_file, std::ios::app);

//   char buffer[1024];
//   ssize_t bytesRead;
//   while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer))) > 0) {
//     std::string chunk(buffer, bytesRead);
//     for (char &c : chunk) {
//       if (c == ',') c = '\t';
//     }
//     // out << chunk;
//     // logging::log() << chunk;
//     std::cout << chunk;
//   }

//   close(pipeOut[0]);
//   // out.close();

//   // Wait for the child process to finish
//   int status;
//   waitpid(child_id, &status, 0);

//   if (WIFEXITED(status)) {
//     std::cout << "Process exited with status " << WEXITSTATUS(status) << "\n";
//   } else if (WIFSIGNALED(status)) {
//     std::cout << "Process killed by signal " << WTERMSIG(status) << "\n";
//   }

//  // Get resource usage
//  struct rusage usage;
//  getrusage(RUSAGE_CHILDREN, &usage);

//  // std::cout << "CPU User Time: " << usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6 << " seconds\n";
//  // std::cout << "CPU System Time: " << usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6 << " seconds\n";
//  // std::cout << "Memory Usage: " << usage.ru_maxrss << " KB\n";
// }

// char* get_time() {
//   auto end = std::chrono::system_clock::now();
//   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
//   return std::ctime(&end_time);
// }

// void child_process(int pipeIn[2], int pipeOut[2], std::string const & exec_name, std::string const & args) {
//   close(pipeIn[1]);
//   close(pipeOut[0]);

//   dup2(pipeIn[0], STDIN_FILENO);
//   dup2(pipeOut[1], STDOUT_FILENO);
//   dup2(pipeOut[1], STDERR_FILENO);

//   close(pipeIn[0]);
//   close(pipeOut[1]);

//   std::string exec = exec_name + " " + args;
//   char * const argv[] = {exec.data(), nullptr};
//   std::cout << "launching " << exec << std::endl;
//   if (execvp(exec_name.data(), argv) == -1) {
//     std::ofstream out;
//     out.open("failure.txt", std::ios::out | std::ios::app);
//     out << get_time() << " failed to execute" << std::endl;
//     throw std::runtime_error("Could not execute command");
//   }
// }

int App::run() {
  ParentArgs parent_args;
  parent_args.input_file = _args.input_file;
  parent_args.output_file = _args.output_file;
  parent_args.log_file = _args.log_file;

  ChildArgs child_args;
  child_args.exec_name = _args.exec_name;
  child_args.exec_args = _args.exec_args;
  sandbox::ProcessLauncher<ParentArgs, ChildArgs> launcher;
  auto ret = launcher.run(parent_process, parent_args, child_process, child_args);
  return ret;
}

}  // end namespace sandbox
