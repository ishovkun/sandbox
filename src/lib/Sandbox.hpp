#pragma once
#include <vector>
#include <string>

namespace sandbox {

class Sandbox {
 public:
  Sandbox() = default;
  int setup(std::vector<std::string>const & allow_directories = {});
};

}  // end namespace sandbox
