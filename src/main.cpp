#include <iostream>
#include "CommandLineArguments.hpp"
#include "App.hpp"

auto main(int argc, char *argv[]) -> int {
  sandbox::CommandLineArguments args;
  try {
    args.parse(argc, argv);
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  // launch
  try {
    sandbox::App app(args);
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}
