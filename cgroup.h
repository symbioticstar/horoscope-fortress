#pragma once

#include <initializer_list>
#include <string>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <csignal>

#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#include "hsc.h"

using std::vector;
using std::string;
using namespace std::filesystem;
using namespace std::chrono_literals;

class cgroup {
 private:
  vector<string> controller_names;
  const string name;
  const path base_dir;

 public:
  cgroup() = delete;
  cgroup(const cgroup &) = delete;
  cgroup &operator=(const cgroup &) = delete;

  constexpr static char cgroup_mount_path[] = "/sys/fs/cgroup";

  explicit cgroup(string name) : name(std::move(name)), base_dir(cgroup_mount_path) {}

  void destroy() {
      using std::cerr, std::endl;
      for (auto &c : controller_names) {
          std::error_code e;
          remove(base_dir / c / name, e); // ignore
          if (e) {
              cerr << string{"DesErr:"} + e.message() << endl;
          }
      }
  }

  void attach_controller(std::string_view controller) {
      controller_names.emplace_back(controller);
      if (mkdir((base_dir / controller / name).c_str(), 0755)) {
          std::cerr << strerror(errno) << std::endl;
          throw hsc_error(HscError::EMkdir);
      }
  }

  void kill_all_procs() const {
      if (!controller_names.empty()) {
          auto p = base_dir / controller_names.front() / name / "tasks";
          while (true) {
              std::ifstream in(p);
              bool exist = false;
              for (pid_t pid; in >> pid;) {
                  exist = true;
                  kill(pid, SIGKILL);
                  waitpid(pid, nullptr, WUNTRACED);
              }
              if (!exist) break;
          }
      }
  }

  void attach_all(pid_t pid) const {
      for (auto &c : controller_names) {
          std::ofstream out(base_dir / c / name / "tasks");
          out << pid << std::endl;
      }
  }

  template<typename T>
  void set_value(const std::string &controller, const std::string &key, const T &val) const {
      std::ofstream out(base_dir / controller / name / key);
      out << val << std::endl;
  }

  template<typename T>
  T get_value(const std::string &controller, const std::string &key) const {
      T val{};
      std::ifstream in(base_dir / controller / name / key);
      in >> val;
      return val;
  }

};

