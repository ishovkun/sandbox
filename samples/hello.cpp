#include <iostream>

auto main() -> int {
  std::string input;

  // Read from standard input and print
  std::getline(std::cin, input);
  std::cout << "Hello " << input << std::endl;

  return EXIT_SUCCESS;
}
