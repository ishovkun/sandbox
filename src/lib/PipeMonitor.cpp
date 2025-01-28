#include "PipeMonitor.hpp"
#include <sys/stat.h>
#include <sys/wait.h>

namespace sandbox {

static bool processAlive(pid_t pid) {
  int status;
  pid_t result = waitpid(pid, &status, WNOHANG);
  if (result == 0) {
    // Child is still running
    return true;
  } else if (result > 0) {
    // Child has exited
    return false;
  } else {
    // Error (e.g., invalid PID)
    throw std::runtime_error("Failed to check child process status");
  }
}

PipeMonitor::PipeMonitor(int proc_id, std::chrono::time_point<std::chrono::high_resolution_clock> start)
    : _procId(proc_id), _start(start)
{
  _epollFd = epoll_create1(0);
  if (_epollFd == -1) {
    throw std::runtime_error("Failed to create epoll instance");
  }
  // Add stdout and stderr pipes to epoll
  _event.events = EPOLLIN;
}

int PipeMonitor::addPipe(int fd, std::ostream &os, bool addTimestamp) {
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
  _pipes.insert({fd, PipeInfo(os, addTimestamp)});
  return 0;
}

void PipeMonitor::start() {
  char buffer[1024];
  struct epoll_event events[2];

  while (processAlive(_procId)) {
    // Periodically check if the child is still alive

    int nfds = epoll_wait(_epollFd, events, _pipes.size(), 100); // Timeout after 100ms
    if (nfds == -1) { // failure, timeout returns 0
      throw std::runtime_error("Error in epoll_wait");
    }

    for (int i = 0; i < nfds; ++i) {
      int fd = events[i].data.fd;
      ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

      if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        auto it = _pipes.find(fd);
        if (it == _pipes.end()) {
          throw std::runtime_error("Pipe not found in map");
        }
        PipeInfo &pipeInfo = it->second;
        if (pipeInfo.add_ts) {
          auto now = std::chrono::high_resolution_clock::now();
          (*pipeInfo.os) << "[" << now << "] ";
        }
        (*pipeInfo.os) << buffer;
      }
    }
  }
}


}  // end namespace sandbox
