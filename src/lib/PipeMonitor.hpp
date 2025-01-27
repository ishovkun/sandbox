#pragma once
#include <ostream>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>

namespace sandbox {

class PipeMonitor
{
 private:
  struct PipeInfo
  {
    std::ostream* os;
    bool add_ts;
    PipeInfo(std::ostream& os, bool stampit) : os(&os), add_ts(stampit) {}
  };

  int _procId{0};
  std::chrono::time_point<std::chrono::high_resolution_clock> _start;
  std::unordered_map<int, PipeInfo> _pipes;
  int _epollFd;
  struct epoll_event _event;
 public:
  PipeMonitor(int proc_id,
              std::chrono::time_point<std::chrono::high_resolution_clock> start =
              std::chrono::high_resolution_clock::now());

  int addPipe(int fd, std::ostream& os, bool addTimestamp);

  void start();
};


}  // end namespace sandbox
