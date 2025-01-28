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
  scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
  if (!ctx) {
    throw std::runtime_error("Failed to initialize seccomp filter");
  }

  // Add rules to forbid mount and umount
  if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mount), 0) != 0) {
    seccomp_release(ctx);
    throw std::runtime_error("Failed to add seccomp rule for mount");
  }

  if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(umount2), 0) != 0) {
    seccomp_release(ctx);
    throw std::runtime_error("Failed to add seccomp rule for umount2");
  }

  // Load the filter into the kernel
  if (seccomp_load(ctx) != 0) {
    seccomp_release(ctx);
    throw std::runtime_error("Failed to load seccomp filter");
  }

  seccomp_release(ctx);
  return 0;
}

static void setup_default_mounts(std::vector<std::pair<std::string,std::string>>const & mount_points) {
  namespace fs = std::filesystem;
  auto root = fs::path("/");
  auto curdir = fs::current_path();
  auto homedir = fs::path(std::getenv("HOME"));
  auto home = homedir.parent_path();

  // Mark all mounts as private to avoid propagating changes to the parent namespace
  assert(mount("none", "/", NULL, MS_REC | MS_SLAVE, NULL) == 0);

  // mount user points
  for (auto const &[real_path, mnt_point] : mount_points) {
    // std::cerr << "bind-mounting " << real_path << " to " << mnt_point << std::endl;
    std::error_code ec;
    std::filesystem::create_directories(mnt_point, ec);
    if (ec) {
      std::cerr << "Failed to create directory " << mnt_point << ": " << ec.message() << std::endl;
      continue;
    }
    if (mount(real_path.c_str(), mnt_point.c_str(), NULL, MS_BIND | MS_RDONLY, NULL) != 0) {
      std::cerr << "Failed to bind-mount " << real_path << " to " << mnt_point
                << std::endl;
    }
  }

  std::filesystem::current_path(root);
  // hide home
  assert(mount("tmpfs", home.c_str(), "tmpfs", 0, NULL) == 0);
  std::filesystem::current_path(root);
  std::error_code ec;
  std::filesystem::current_path(home, ec);
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

	assert(unshare(CLONE_NEWUSER) == 0);
	become_uid_orig(my_uid, my_gid);

  setup_seccomp();
  return 0;
}


}  // end namespace sandbox
