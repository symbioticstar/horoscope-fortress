#pragma once

#include <fcntl.h>
#include <random>
#include <cstring>
#include <fstream>
#include <csignal>

constexpr size_t default_size = 256;
constexpr auto version = "1.0";

enum class HscError {
  Ok = 0,
  EKafel,
  EPrctl,
  EChild,
  EUrandom,
  ECgroup,
  EWait,
  ESetUid,
  ESetGid,
  ESeccomp,
  ESeccompLoad,
  EBadArgs,
  EOpen,
  EOpen2,
  EOpen3,
  EOpenA,
  ERmdir,
  EPipe,
  EWritePipe,
  EReadPipe,
  EUnshare,
  EMmap,
  EExec,
  EDup2Stdin = 64,
  EDup2Stdout,
  EDup2Stderr,
  EDup2Etc,
  EMkdir,
  EChdir,
  ECgroupGetVal = 128,
  EUnknown = 256,
  EStdException
};

uint64_t random_key() {
    std::ifstream in("/dev/urandom");
    uint64_t ret = 0;
    for (int i = 0; i < 16; i++) {
        ret = ret * 131 + in.get();
    }
    return ret;
}

class hsc_error : std::exception {
 public:
  HscError error;
  int err;
  explicit hsc_error(HscError error) : error(error), err(errno) {}
  [[nodiscard]] const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
      return strerror(err);
  }
};

auto clone_flag = CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID | SIGCHLD | CLONE_NEWNET | CLONE_NEWPID;

#include <memory>
#include <functional>
#include <cstdio>
#include <sstream>
#include <unistd.h>

using defer = std::shared_ptr<void>;

std::string run();
template<typename T, size_t buffer_size = 20>
std::optional<T> read_from_fd(int fd) {
    auto buffer = std::make_unique<char[]>(buffer_size);
    auto ret = read(fd, buffer.get(), buffer_size);
    if (ret <= 0) return std::nullopt;
    std::stringstream ss(buffer.get());
    T val;
    ss >> val;
    return val;
}

template<typename T>
void write_to_fd(int fd, T val) {
    auto str = std::to_string(val);
    if (write(fd, str.c_str(), str.length()) < -1) throw hsc_error(HscError::EWritePipe);
}



