#pragma once
#include <functional>
#include <array>
#include <unistd.h> // pipe
#include <iostream>
#include <seccomp.h>
#include <sys/types.h>
#include <unistd.h>

namespace sandbox {

struct LambdaWrapper {
    std::function<int(void*)> func;
    void* args;
};

// Static wrapper to call the lambda
int lambdaInvoker(void* wrapperPtr) {
    auto* wrapper = static_cast<LambdaWrapper*>(wrapperPtr);
    return wrapper->func(wrapper->args);
}

int setup_seccomp() {
    // Initialize the seccomp filter
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL); // Kill process if syscall is not allowed
    if (ctx == NULL) {
        perror("seccomp_init");
        return -1;
    }

    // Allow specific system calls
    if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0) < 0 ||
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0) < 0 ||
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0) < 0 ||
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0) < 0) {
        perror("seccomp_rule_add");
        seccomp_release(ctx);
        return -1;
    }

    // Load the seccomp filter
    if (seccomp_load(ctx) < 0) {
        perror("seccomp_load");
        seccomp_release(ctx);
        return -1;
    }

    // Free the filter context
    seccomp_release(ctx);
    return 0;
}

template <typename ParentArgs, typename ChildArgs>
class IsolatedProcessLauncher {
 public:
  IsolatedProcessLauncher() = default;

  void run(std::function<int(ParentArgs&)> parentFunc,
           ParentArgs &parentArgs,
           std::function<int(ChildArgs&)> childFunc,
           ChildArgs& childArgs) {

    // setup pipes
    std::array<int,2> pipeIn{-1, -1}, pipeOut{-1, -1};
    if (pipe(pipeIn.data()) == -1 || pipe(pipeOut.data()) == -1) {
      throw std::runtime_error("Could not create pipes");
    }
    parentArgs.pipeIn = pipeIn;
    parentArgs.pipeOut = pipeOut;
    childArgs.pipeIn = pipeIn;
    childArgs.pipeOut = pipeOut;

    int pid = fork();
    if (pid == 0) {
      if (setup_seccomp() < 0) {
        fprintf(stderr, "Failed to set up seccomp\n");
        exit(EXIT_FAILURE);
      }
      // scmp_filter_ctx ctx;
      // ctx = seccomp_init(SCMP_ACT_ALLOW);
      // child process
      childFunc(childArgs);
    } else {
      // parent process
      parentFunc(parentArgs);
    }

    // // Allocate stack for `clone`
    // constexpr size_t STACK_SIZE = 1024 * 1024;
    // char *stack = new char[STACK_SIZE];
    // if (!stack) {
    //     // logError(logFile, "Failed to allocate stack memory.");
    //   throw std::runtime_error("Failed to allocate stack memory.");
    // }
    // char *stackTop = stack + STACK_SIZE;



    // auto childWrapper = [=](void* args) -> int {
    //   // scmp_filter_ctx ctx;
    //   // ctx = seccomp_init(SCMP_ACT_ALLOW);
    //   // unshare(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS);
    //   // Unmount everything
    //   // unmountAll();
    //   // childFunc(reinterpret_cast<ChildArgs>(*args));
    //   return 0;
    // };
    //  LambdaWrapper wrapper{
    //     .func = childWrapper,
    //     .args = (void*)"Hello from child!",
    // };

    // // pid_t pid = clone(childFunc, stackTop, CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | SIGCHLD, &childArgs);
    // // int pid=0;
    // pid_t pid = clone(lambdaInvoker, stackTop, CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | SIGCHLD, &wrapper);
    // parentArgs.childId = pid;
    // if (pid == -1) {
    //     // logError(logFile, "Failed to create new namespace: " + std::string(strerror(errno)));
    //     std::cerr <<  "Failed to create new namespace:" << std::endl;
    //     delete[] stack;
    //     throw std::runtime_error("Failed to create new namespace");
    // }
    // if (pid > 0) {
    //   parentFunc(parentArgs);
    // }
  }
};

}  // end namespace sandbox
