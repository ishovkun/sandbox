#pragma once
#include "CommandLineArguments.hpp"

namespace sandbox {

/* This class is the main class that implements the executable.
 * It copies cli arguments to the parent and child argument structs
 * and launches the executions. */
class App
{
  CommandLineArguments _args;

 public:
  App(CommandLineArguments const & args);

  int run();

 private:
};


}  // end namespace sandbox
