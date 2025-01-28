#include <fstream>
#include <filesystem>
#include <string.h>
#include <unistd.h>  // close
#include <sys/wait.h>
#include <iostream>  // debug
#include "ProcessAPI.hpp"
#include "PipeMonitor.hpp"

namespace sandbox {

static void close_unused_pipes(ParentArgs & arg) {
  close(arg.pipeIn[0]);
  close(arg.pipeOut[1]);
  close(arg.pipeErr[1]);
}

static void close_remaining_pipes(ParentArgs & arg) {
  close(arg.pipeIn[1]);
  close(arg.pipeOut[0]);
  close(arg.pipeErr[0]);
}

std::string read_input(std::string const & input_file) {
  // check read permissions
  namespace fs = std::filesystem;
  if (!fs::is_regular_file(input_file) ||
      (fs::status(input_file).permissions() & fs::perms::owner_read) == fs::perms::none) {
    throw std::invalid_argument("Input file is not executable");
  }
  std::ifstream iss(input_file);
  if (!iss.is_open()) {
    throw std::runtime_error("Could not open input file");
  }
  std::ostringstream buf;
  buf << iss.rdbuf();
  return buf.str();
}

int waitForChild(pid_t pid) {
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  } else {
    return -1;
  }
}

auto parent_process(ParentArgs & arg) -> int {
  close_unused_pipes(arg);

  // wire up communication
  sandbox::PipeMonitor monitor(arg.childId);
  std::ofstream fout(arg.output_file);
  std::ofstream ferr(arg.log_file);
  if (0 != monitor.addPipe(arg.pipeOut[0], fout, /*timestamp*/ false))
    return false;
  if (0 != monitor.addPipe(arg.pipeErr[0], ferr, /*timestamp*/ true))
    return false;

  // post input to child
  auto input = read_input(arg.input_file);
  write(arg.pipeIn[1], input.c_str(), input.size());

  monitor.start();

  close_remaining_pipes(arg);

  auto ret = waitForChild(arg.childId);
  if (ret != 0) {
    ferr << "Child process failed with exit code " << ret
         << std::string(strerror(errno))
         << std::endl;
  }
  return ret;
}

}  // end namespace sandbox
