#pragma once
#include "CommandLineArguments.hpp"

namespace sandbox {

class Sandbox
{
  CommandLineArguments _args;
 public:
  Sandbox(CommandLineArguments const & args);

  void launch();

 private:
};


}  // end namespace sandbox
