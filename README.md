# Sandbox
The  Sandbox Project  provides  a minimal  lightweight  sandboxing solution  for
isolating  and  managing  processes.  The sandbox  leverages  Linux  namespaces,
seccomp filtering, and custom filesystem mounts to restrict processes' access to
resources, ensuring security and stability.

## Features

- Process Isolation: Use Linux namespaces to isolate processes from the host environment.
- Filesystem Restriction: Mount minimal filesystems and bind-mount specific directories for controlled access.
- Syscall Filtering: Employ seccomp filters to restrict system calls, enhancing security by limiting attack surfaces.
- Pipe-based Communication: Route stdout and stderr from the child process to the parent for logging and output separation.
- Time-Stamped Error Logs: Capture and log errors from the child process with precise timestamps.
- Resource Monitoring: Measure CPU usage, memory utilization, and other resource metrics for both the sandbox and the isolated process.

# seccomp libseccomp-dev
