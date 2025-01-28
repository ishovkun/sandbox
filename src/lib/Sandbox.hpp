#pragma once
#include <vector>
#include <string>

namespace sandbox {

class Sandbox {
 public:
  Sandbox() = default;
  // int setup(std::vector<std::string>const & allow_directories = {});
  int setup(std::vector<std::pair<std::string,std::string>>const & mount_points = {});
};

}  // end namespace sandbox
