#include "PipeMonitor.hpp"
#include <sys/stat.h>
#include <sys/wait.h>

namespace sandbox {

PipeMonitor::PipeMonitor(int proc_id)
    : _procId(proc_id)
{
  _epollFd = epoll_create1(0);
  if (_epollFd == -1) {
    throw std::runtime_error("Failed to create epoll instance");
  }
  // Add stdout and stderr pipes to epoll
  _event.events = EPOLLIN;
}

int PipeMonitor::addPipe(int fd, std::ostream& os, WriterFunc const& editor) {
  // check that pipe is open
  struct stat statbuf;
  if (fstat(fd, &statbuf) != 0)
    return 1;

  if (!os.good())
    return 2;

  _event.data.fd = fd;
  if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &_event) == -1) {
    return 3;
  }
  _pipes.insert({fd, PipeInfo(os, editor)});
  return 0;
}

void PipeMonitor::start() {
  char buffer[1024];
  struct epoll_event events[2];

  int num_open_pipes = _pipes.size();
  while (num_open_pipes > 0) {

    int nfds = epoll_wait(_epollFd, events, _pipes.size(), 100); // Timeout after 100ms
    if (nfds == -1) { // failure, timeout returns 0
      throw std::runtime_error("Error in epoll_wait");
    }

    for (int i = 0; i < nfds; ++i) {
      int const fd = events[i].data.fd;
      auto const bytesRead = read(fd, buffer, sizeof(buffer) - 1);

      if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        auto it = _pipes.find(fd);

        // sanity check
        if (it == _pipes.end()) {
          throw std::runtime_error("Pipe not found in map");
        }

        PipeInfo &pipeInfo = it->second;
        pipeInfo.writer(*pipeInfo.os, buffer, bytesRead);
      }
      else {
        num_open_pipes--;
      }
    }
  }
}


}  // end namespace sandbox
