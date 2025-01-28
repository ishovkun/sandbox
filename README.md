# Sandbox
The  Sandbox Project  provides  a minimal  lightweight  sandboxing solution  for
isolating  and  managing  processes.  The sandbox  leverages  Linux  namespaces,
seccomp filtering, and custom filesystem mounts to restrict processes' access to
resources, ensuring security and stability. No sudo is required (unless you're on an
Azure VM that does some filesystem trickery that won't let my code run :-( )!

## Features

- Process Isolation: Use Linux namespaces to isolate processes from the host environment.
- Filesystem Restriction: Mount minimal filesystems and bind-mount specific directories for controlled access.
- Syscall Filtering: Employ seccomp filters to restrict system calls, enhancing security by limiting attack surfaces.
- Pipe-based Communication: Route stdout and stderr from the child process to the parent for logging and output separation.
- Time-Stamped Error Logs: Capture and log errors from the child process with precise timestamps.
- Resource Monitoring: Measure CPU usage, memory utilization, and other resource metrics for both the sandbox and the isolated process.

## Building the project
### Prerequisites
- Linux kernel (minimum 3.8) with namespace and seccomp support.
- A recent GCC or Clang compiler that support C++-20 (GCC 10 or later Clang 12 or later).
- Optionally, install seccomp development libraries (libseccomp-dev on Ubuntu).

### Build instructions
1. Clone the repository
``` sh
git clone https://github.com/ishovkun/sandbox
cd standbox
```
2. Build the project

``` sh
mkdir build && cd build
cmake ..
make
```
3. Run tests
``` sh
./tests
```
4. Run sanbox: 
Running sandbox requires issuing a command like
``` sh
./sandbox input_file output_file log_file path-to-executable [executable-args...]
```
where the last argument is optional. For example, you can try running one of the examples
that is compiled during the build as follows:
``` sh
echo "$USER" >> input.txt
./sandbox input.txt output.txt log.txt samples/hello 
```
Then, you should see output.txt and log.txt in your working direcoty. log.txt will be empty, and
output.txt will contain a message greeting you.


## How it works
The program starts and parses the user input. Next, the program issues a fork() system call to create a 
child process.

Three pipes are created to 
- Let the parent process write into child's stdin.
- Forward the child's stdout and stderr to the parent.

The parent process reads the input file and sends the data into the pipe to be read by the child.
Next, the parent process opens two files for writing the output and log (errors).
This code uses epoll_wait to monitor the events in the pipes to be able to resolve errors that occur within less than 1 ms.

The child process uses dup2 to route stdout and stderr into the output pipes and routes the input pipe into stdin.
Next, it uses unshare() to create its own namespace and sets up the filesystem accordingly.
Specifically, it first assigns its uid and gid to 0, and remounts parts of the filesystem.
It bind-mounts the directory with the executable file to "/tmp", and also mounts
a temporary filesystem tmpfs into "/home" in order to effectively hide the user data.
Then, the child process assigns its uid and gid to the original values.
Optionally, the child process enforces seccomp rules to block mount and umount system calls.
Finally, the child process executes the execvp() system call to launch the sandboxed program.

## Potential improvements

- More fine-grained control of seccomp could be implemented
- cgroups could be use to prevent resource exhaustion (e.g., fork bombs)
- The code could benefit from an improved logging
- A proper test suite can be implemented instead of a simple ad-hoc solution
- An overlay filesystem can be used instead of tempfs to provide writable layers while preserving the base system structure.
