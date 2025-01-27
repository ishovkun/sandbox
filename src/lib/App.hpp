#pragma once
#include "CommandLineArguments.hpp"

namespace sandbox {

class App
{
  CommandLineArguments _args;
 public:
  App(CommandLineArguments const & args);

  void launch();

 private:
};


}  // end namespace sandbox
