#include <seccomp.h>
#include <stdexcept>
#include <iostream>
#include <sys/mount.h>
#include <string.h>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <format>
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

static void setup_mounts(void) {
  auto root = std::filesystem::path("/");
  auto curdir = std::filesystem::current_path();
  auto homedir = std::filesystem::path(std::getenv("HOME"));

  assert(mount("none", "/", NULL, MS_REC | MS_SLAVE, NULL) == 0);

  std::filesystem::current_path(homedir);
  // hide home
  assert(mount("tmpfs", (homedir).c_str(), "tmpfs", MS_RDONLY, NULL) == 0);
  // cd path
  std::filesystem::current_path(root);
  // std::filesystem::current_path(curdir);
}

static void write_file(std::string const & file_name, std::string const & content) {
  std::ofstream file;
  file.open(file_name);
  file << content;
  file.close();
}

static void deny_setgroups(void)
{
  write_file("/proc/self/setgroups", "deny\n");
}

static void self_map_uid(uid_t from, uid_t to) {
  write_file("/proc/self/uid_map", std::format("{0} {1} 1\n", from, to));
}

static void self_map_gid(gid_t from, gid_t to) {
  write_file("/proc/self/gid_map", std::format("{0} {1} 1\n", from, to));
}

static void become_uid0(uid_t orig_uid, gid_t orig_gid)
{
  deny_setgroups();
  self_map_uid(0, orig_uid);
  self_map_gid(0, orig_gid);
  assert(setuid(0) == 0);
  assert(setgid(0) == 0);
}

static void become_uid_orig(uid_t orig_uid, gid_t orig_gid)
{
  self_map_uid(orig_uid, 0);
  self_map_gid(orig_gid, 0);
  assert(setuid(orig_uid) == 0);
  assert(setgid(orig_gid) == 0);
}

void write(std::string const& filename, std::string const& content) {
  std::ofstream file;
  file.open(filename);
  file << content;
  file.close();
}

int Sandbox::setup()
{
	char *const new_argv[] = {"/bin/bash", NULL};
	uid_t my_uid = getuid();
	gid_t my_gid = getgid();

	if (unshare(CLONE_NEWNS | CLONE_NEWUSER) != 0) {
    throw std::runtime_error("Failed to unshare namespaces: " +
                             std::string(strerror(errno)));
  }
	become_uid0(my_uid, my_gid);
	setup_mounts();

	assert(unshare(CLONE_NEWUSER) == 0);
	become_uid_orig(my_uid, my_gid);

	return execvp(new_argv[0], new_argv);
  return 0;
}


}  // end namespace sandbox
