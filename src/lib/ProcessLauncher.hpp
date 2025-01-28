#pragma once
#include <functional>
#include <array>
#include <unistd.h> // pipe
#include <iostream> // debug

namespace sandbox {

/*
** This is a wrapper class for launching a child process from a parent process.
** It saves the pipe descriptors in the parent and child arguments.
** It then launches a child process that executes the child function.
** The parent executes the parent function.
 */
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
      parentArgs.childId = pid;
      return parentFunc(parentArgs);
    }
  }
};

}  // end namespace sandbox
