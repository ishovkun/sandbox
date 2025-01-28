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
{}

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
