#pragma once
#include <ostream>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <functional>

namespace sandbox {

using WriterFunc = std::function<void(std::ostream&, char*, int)>;

class PipeMonitor
{
 private:
  struct PipeInfo
  {
    std::ostream* os;
    WriterFunc writer;
    PipeInfo(std::ostream& os, WriterFunc writer) : os(&os), writer(writer) {}
  };

  int _procId{0};
  std::unordered_map<int, PipeInfo> _pipes;
  int _epollFd;
  struct epoll_event _event;
 public:
  PipeMonitor(int proc_id);

  int addPipe(int fd, std::ostream& os, WriterFunc const& editor);

  void start();
};


}  // end namespace sandbox
