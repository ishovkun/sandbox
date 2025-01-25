#pragma once

namespace sandbox {

struct ParentArgs {
  std::array<int, 2> pipeIn;
  std::array<int, 2> pipeOut;
};

}  // end namespace sandbox
