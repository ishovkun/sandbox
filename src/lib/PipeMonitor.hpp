#pragma once
#include <ostream>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <functional>

namespace sandbox {

using WriterFunc = std::function<void(std::ostream&, char*, int)>;

/*
** This class monitors the pipes between the current process and
** a specified process.
*/
class PipeMonitor
{
 private:
  // holds the components of writing into the output stram
  struct PipeInfo
  {
    std::ostream* os;
    WriterFunc writer;
    PipeInfo(std::ostream& os, WriterFunc writer) : os(&os), writer(writer) {}
  };

  int _procId{0};                           // id of the process to monitor
  std::unordered_map<int, PipeInfo> _pipes; // map pid -> corresponding pipe
  int _epollFd;                             // descriptor of the epoll
  struct epoll_event _event;

 public:
  // Contstructor, sotres the pid of the process to monitor
  PipeMonitor(int proc_id);
  // map the pipe to the corresponding output stream and writer function
  int addPipe(int fd, std::ostream& os, WriterFunc const& writer);
  // start monitoring
  // exists then all pipes are done
  // NOTE: does not manage (i.e. open/close) the pipes itself.
  void start();
};


}  // end namespace sandbox
