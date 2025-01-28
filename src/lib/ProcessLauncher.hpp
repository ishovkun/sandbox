#pragma once
#include <functional>
#include <array>
#include <unistd.h> // pipe

namespace sandbox {

template <typename ParentArgs, typename ChildArgs>
class ProcessLauncher {
 public:
  ProcessLauncher() = default;

  int run(std::function<int(ParentArgs&)> parentFunc,
           ParentArgs &parentArgs,
           std::function<void(ChildArgs&)> childFunc,
           ChildArgs& childArgs) {

    std::array<int,2> pipeIn{-1, -1}, pipeOut{-1, -1}, pipeErr{-1, -1};
    if (pipe(pipeIn.data()) == -1 ||
        pipe(pipeOut.data()) == -1 ||
        pipe(pipeErr.data()) == -1) {
      throw std::runtime_error("Could not create pipes");
    }
    parentArgs.pipeIn = pipeIn;
    parentArgs.pipeOut = pipeOut;
    parentArgs.pipeErr = pipeErr;
    childArgs.pipeIn = pipeIn;
    childArgs.pipeOut = pipeOut;
    childArgs.pipeErr = pipeErr;

    int pid = fork();
    if (pid == 0) {
      // child process
      childFunc(childArgs);
      return -1;
    } else {
      // parent process
      return parentFunc(parentArgs);
    }
  }
};

}  // end namespace sandbox
