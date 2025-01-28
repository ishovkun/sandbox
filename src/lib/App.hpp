#pragma once
#include "CommandLineArguments.hpp"

namespace sandbox {

class App
{
  CommandLineArguments _args;
 public:
  App(CommandLineArguments const & args);

  int run();

 private:
};


}  // end namespace sandbox
