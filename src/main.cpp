#include <iostream>
#include <sys/resource.h>
#include "CommandLineArguments.hpp"
#include "App.hpp"

auto print_resource_utilization() -> void {
  struct rusage usage;
  getrusage(RUSAGE_CHILDREN | RUSAGE_SELF, &usage);
  std::cout << "CPU User Time: " << usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6 << " seconds\n";
  std::cout << "CPU System Time: " << usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6 << " seconds\n";
  std::cout << "Memory Usage: " << usage.ru_maxrss << " KB\n";
}

auto print_usage_example() -> void {
  std::cerr << "Example Usage:\n";
  std::cerr << "sandbox input_file output_file log_file <path/to/executable> [executable args...]"
            << std::endl;
}

auto main(int argc, char *argv[]) -> int {
  sandbox::CommandLineArguments args;
  try {
    args.parse(argc, argv);
  }
  catch (const std::exception &e) {
    std::cerr << "Error occured during input parsing:" << std::endl;
    std::cerr << e.what() << std::endl;
    print_usage_example();
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

  print_resource_utilization();
  return EXIT_SUCCESS;
}
