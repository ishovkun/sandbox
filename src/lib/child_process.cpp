#include <stdexcept>
#include <unistd.h>  // close
#include <iostream>
#include <filesystem>
#include "ProcessAPI.hpp"
#include "Sandbox.hpp"

/*
** This file implements the child process.
*/

namespace sandbox {

static void check_can_execute(std::string const & file) {
  namespace fs = std::filesystem;
  if (!fs::exists(file)) {
    throw std::invalid_argument("Executable does not exist " + file);
  }
  if (!std::filesystem::is_regular_file(file) ||
      (std::filesystem::status(file).permissions() & fs::perms::owner_exec)
      == fs::perms::none) {
    throw std::invalid_argument("Input file is not executable");
  }
}

void setup_pipes(ChildArgs & arg) {
  close(arg.pipeIn[1]);
  close(arg.pipeOut[0]);
  close(arg.pipeErr[0]);

  dup2(arg.pipeIn[0], STDIN_FILENO);
  dup2(arg.pipeOut[1], STDOUT_FILENO);
  dup2(arg.pipeErr[1], STDERR_FILENO);

  close(arg.pipeIn[0]);
  close(arg.pipeOut[1]);
  close(arg.pipeErr[1]);
}

// this function implements the clid process logic
// it sets up the pipes (STDIN, STDERR, STDOUT)
// Next, it sandboxes the execution contexts and launches
// the executable.
auto child_process(ChildArgs & arg) -> int {
  setup_pipes(arg);

  namespace fs = std::filesystem;
  std::string exec_dir = fs::path(arg.exec_name).parent_path();
  auto cur_dir = fs::current_path();

  // we bind-mount the executable into /tmp
  fs::path exec_mnt_point = "/tmp/";
  std::string exec_mnt_path = exec_mnt_point / fs::path(arg.exec_name).filename();
  auto exec_name = exec_mnt_path;

  sandbox::Sandbox sandbox;
  if (sandbox.setup({{exec_dir, exec_mnt_point}}) != 0)
    throw std::runtime_error("sandbox setup failed");

  std::string exec = exec_name + " " + arg.exec_args;
  char * const argv[] = {exec.data(), nullptr};

  check_can_execute(exec_name);

  auto ret = execvp(exec_name.data(), argv);
  if (ret == -1) {
    std::cerr << "Exec failed: " << arg.exec_name << std::endl;
  }
  return ret;
}

}  // end namespace sandbox
