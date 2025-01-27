#include <unistd.h> // pipe
#include <seccomp.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <sys/mount.h>
#include <string.h>
#include <fstream>
#include <sys/syscall.h>
#include <sched.h>
#include <linux/prctl.h>
#include <cassert>
#include <cstdarg>
#include <grp.h>
// #include <sys/capability.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "Sandbox.hpp"

namespace sandbox {

static int setup_seccomp() {
  // Initialize the seccomp filter
  scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL); // Kill process if syscall is not allowed
  if (ctx == NULL) {
    throw std::runtime_error("Failed to initialize seccomp");
  }

  // Allow specific system calls
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(execve), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);

  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getdents), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getdents64), 0); // Needed for directory iteration
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(stat), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(statx), 0); // Needed for file metadata

  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);

  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(access), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getcwd), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fcntl), 0);
  // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(), 0);

  // Load the seccomp filter
  if (seccomp_load(ctx) < 0) {
    throw std::runtime_error("Failed to load seccomp filter");
  }
  // std::cout << "bad" << std::endl;

  // Free the filter context
  // seccomp_release(ctx);
  return 0;
}

static void write_proc_self(const char *file, const char *content)
{
  size_t len = strlen(content) + 1;
  char *path;
  int fd;

  assert(asprintf(&path, "/proc/self/%s", file) != -1);
  std::cout << "open " << path << std::endl;
  fd = open(path, O_RDWR | O_CLOEXEC);
  if (fd < 0)
    throw std::runtime_error("Failed to open file "+ std::string(strerror(errno)));

  free(path);
  assert(fd >= 0);
  assert(write(fd, content, len) == len);
  close(fd);
}

static void setup_mounts(void) {
  char *curdir = get_current_dir_name();
  char *homedir = getenv("HOME");
  char *sshdir;

  assert(curdir);
  assert(homedir);
  assert(mount("none", "/", NULL, MS_REC | MS_SLAVE, NULL) == 0);

  assert(asprintf(&sshdir, "%s/.ssh", homedir) != -1);
  assert(mount("tmpfs", sshdir, "tmpfs", MS_RDONLY, NULL) == 0);
  chdir("/");
  chdir(curdir);

  free(sshdir);
  free(curdir);
}

static void update_uidgid_map(uid_t from, uid_t to, bool is_user)
{
  char *map_content;

  assert(asprintf(&map_content, "%u %u 1\n", from, to) != -1);

  if (is_user)
    write_proc_self("uid_map", map_content);
  else
    write_proc_self("gid_map", map_content);

  free(map_content);
}

static void deny_setgroups(void)
{
  write_proc_self("setgroups", "deny\n");
}

static void become_uid0(uid_t orig_uid, gid_t orig_gid)
{
  deny_setgroups();
  update_uidgid_map(0, orig_gid, false);
  update_uidgid_map(0, orig_uid, true);
  assert(setuid(0) == 0);
  assert(setgid(0) == 0);
}


int setup_files() {

  // assert(unshare(CLONE_NEWUSER) == 0);


  // // auto flags = CLONE_NEWUTS | CLONE_NEWNS;
  // auto flags = CLONE_NEWPID | CLONE_NEWNS|CLONE_NEWUTS| CLONE_NEWIPC|CLONE_NEWUSER;
  // // auto flags =  CLONE_NEWPID;
  // // auto flags = CLONE_NEWNS | CLONE_FS;
  // if (unshare(flags) < 0) {
  //   throw std::runtime_error("Failed to unshare namespaces");
  // }

  // uid_t my_uid = getuid();
  // gid_t my_gid = getgid();
  // std::cout << "uid = " << my_uid << std::endl;
  // std::cout << "gid = " << my_gid << std::endl;
  // become_uid0(my_uid, my_gid);
  // setup_mounts();

  // mkdir("/tmp/newroot", 0755);

// Change the root directory to the new location

  // if (mount("/", "/", "none", MS_PRIVATE|MS_REC, nullptr) == -1)
  //   throw std::runtime_error("mount --make-rprivate /");
  // mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL);
  if (mount("none", "/", NULL, MS_PRIVATE, NULL) < 0)
    throw std::runtime_error("Failed to bind mount " + std::string(strerror(errno)));


  // const char *sandbox = "/home/ishovkun/dev/sandbox/build/nsp";
  // if (mount("/proc", "/proc", "proc", MS_BIND|MS_NOSUID, NULL) < 0)
  //   throw std::runtime_error("Failed to bind mount proc " + std::string(strerror(errno)));
  // // Try to unmount the root filesystem
  // // if (umount2("/", MNT_DETACH) < 0) {
  // if (umount2("/home", MNT_DETACH) < 0) {
  //   throw std::runtime_error("Failed to unmount /: " + std::string(strerror(errno)));
  // }
  // Unmount everything and remount /proc
  // if (umount2("/", MNT_DETACH) < 0) throw std::runtime_error("umount /");
  // if (mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) throw std::runtime_error("remount root private");
  // if (mount("tmpfs", "/tmp", "tmpfs", 0, NULL) < 0) throw std::runtime_error("mount tmpfs");
  return 0;
}

static void become_orig(uid_t orig_uid, gid_t orig_gid)
{
  update_uidgid_map(orig_gid, 0, false);
  update_uidgid_map(orig_uid, 0, true);
  assert(setuid(orig_uid) == 0);
  assert(setgid(orig_gid) == 0);
}

void write_to_file(const char *which, const char *format, ...) {
  FILE * fu = fopen(which, "w");
  va_list args;
  va_start(args, format);
  if (vfprintf(fu, format, args) < 0) {
    perror("cannot write");
    exit(1);
  }
  fclose(fu);
}

void write(std::string const& filename, std::string const& content) {
  std::ofstream file;
  file.open(filename);
  file << content;
  file.close();
}

int Sandbox::setup()
{
  // char *const new_argv[] = {"/bin/bash", NULL};
  uid_t uid = getuid();
  gid_t gid = getgid();

  // auto flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWUSER;
  auto flags = CLONE_NEWUSER | CLONE_FS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_FILES;
  if (unshare(flags) != 0) {
    throw std::runtime_error("Failed to unshare namespaces");
  }


  setgid(0);
  setgroups(0, NULL);
  setuid(0);

  std::cout << "write uid map" << std::endl;
  write("/proc/self/uid_map", "0 " + std::to_string(uid) + " 1\n");
  // deny setgroups (see user_namespaces(7))
  std::cout << "write set groups" << std::endl;
  write("/proc/self/setgroups", "deny\n");
  // remap gid
  std::cout << "write set gid" << std::endl;
  write("/proc/self/gid_map", "0 " + std::to_string(gid) + " 1\n");
  std::cout << "done" << std::endl;


  if (umount2("/home", MNT_DETACH) < 0) {
    throw std::runtime_error("Failed to unmount /home: " + std::string(strerror(errno)));
  }
  // if (setuid(0) != 0) {
  //   throw std::runtime_error("Failed set uid " + std::string(strerror(errno)));
  // }
  // assert(setgid(0) == 0);

  // if (mount("none", "/", NULL, MS_PRIVATE, NULL) < 0)
  //   throw std::runtime_error("Failed to bind mount " + std::string(strerror(errno)));

  // mkdir("/tmp/newroot", 0755);

  // std::cout << "new uid = " << getuid() << std::endl;
  // std::cout << "new gid = " << getgid() << std::endl;
  // exit(0);
  // become_uid0(my_uid, my_gid);
  // std::cout << "uid = " << getuid() << std::endl;
  // std::cout << "gid = " << getgid() << std::endl;

  // setup_mounts();

  // assert(unshare(CLONE_NEWUSER) == 0);
  // become_orig(my_uid, my_gid);

  // // return execvp(new_argv[0], new_argv);

  // // setup_files();
  // // setup_seccomp();
  // // if (setup_seccomp() < 0) {
  // //   fprintf(stderr, "Failed to set up seccomp\n");
  // //   exit(EXIT_FAILURE);
  // // }
  return 0;
}


}  // end namespace sandbox
