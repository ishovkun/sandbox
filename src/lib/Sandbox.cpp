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

static void setup_default_mounts(std::vector<std::pair<std::string,std::string>>const & mount_points) {
  auto root = std::filesystem::path("/");
  auto curdir = std::filesystem::current_path();
  auto homedir = std::filesystem::path(std::getenv("HOME"));

  // Mark all mounts as private to avoid propagating changes to the parent namespace
  assert(mount("none", "/", NULL, MS_REC | MS_SLAVE, NULL) == 0);

  // Remount /mnt as read-write to allow directory creation
  // if (mount(NULL, "/mnt", NULL, MS_REMOUNT | MS_PRIVATE | MS_REC, NULL) < 0) {
  //   std::cerr << "Failed to remount /mnt as private and writable: " << strerror(errno) << std::endl;
  //   throw std::runtime_error("Failed to remount /mnt");
  // }

  // mount user points
  for (auto const &[real_path, mnt_point] : mount_points) {
    std::cerr << "bind-mounting " << real_path << " to " << mnt_point << std::endl;
    std::error_code ec;
    std::filesystem::create_directories(mnt_point, ec);
    if (ec) {
      std::cerr << "Failed to create directory " << mnt_point << ": " << ec.message() << std::endl;
      continue;
    }
    if (mount(real_path.c_str(), mnt_point.c_str(), NULL, MS_BIND, NULL) != 0) {
      std::cerr << "Failed to bind-mount " << real_path << " to " << mnt_point
                << std::endl;
    }
  }

  std::filesystem::current_path(homedir);
  // hide home
  assert(mount("tmpfs", homedir.c_str(), "tmpfs", MS_RDONLY, NULL) == 0);
  // for (auto const & dir : mount_directories)
  //   assert(mount(dir.c_str(), dir.c_str(), NULL, MS_BIND, NULL) == 0);
  // cd root
  std::filesystem::current_path(root);
  // try to get back into current directory if it is mounted
  std::error_code ec;
  std::filesystem::current_path(curdir, ec);
}

// static void setup_user_mounts(std::vector<std::pair<std::string,std::string>>const & mount_points)
// {
//   for (auto const & [real_path, mnt_point] : mount_points)
//     if (mount(mnt_point.c_str(), real_path.c_str(), NULL, MS_BIND, NULL) != 0) {
//       std::cerr << "Failed to mount " << real_path << " to " << mnt_point << std::endl;
//     }
// }


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

int Sandbox::setup(std::vector<std::pair<std::string,std::string>>const & mount_points)
{
	uid_t my_uid = getuid();
	gid_t my_gid = getgid();

	if (unshare(CLONE_NEWNS | CLONE_NEWUSER) != 0) {
    throw std::runtime_error("Failed to unshare namespaces: " +
                             std::string(strerror(errno)));
  }
	become_uid0(my_uid, my_gid);
	setup_default_mounts(mount_points);
  // setup_user_mounts(mount_points);

	assert(unshare(CLONE_NEWUSER) == 0);
	become_uid_orig(my_uid, my_gid);

	// char *const new_argv[] = {"/bin/bash", NULL};
	// return execvp(new_argv[0], new_argv);
  return 0;
}


}  // end namespace sandbox
