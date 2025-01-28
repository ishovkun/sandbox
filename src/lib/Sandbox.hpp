#pragma once
#include <vector>
#include <string>

namespace sandbox {

// This class implements the sandboxing logic
// It isolates the process and does the filesystem magic
// Additionally, it can enforce seccomp rules
class Sandbox {
 public:
  Sandbox() = default;
  // This is the main funciton.
  // The mount_points argument specifies the mapping
  // from the real filesystem to the mounted points
  int setup(std::vector<std::pair<std::string,std::string>>const & mount_points = {});
};

}  // end namespace sandbox
