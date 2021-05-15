#pragma once

#include "httplib.h"
#include "../hsc.h"

#include <utility>
#include <vector>
#include <tuple>
#include <string>
#include <optional>
#include <sys/wait.h>

using std::vector, std::string, std::tuple;
using std::optional, std::nullopt;
using namespace std::literals::string_literals;
using str = const char *;

const vector<tuple<str, str, str>> latern_mapping = {
    {"cwd", "cwd", nullptr},
    {"stdin_path", "stdin-path", "/dev/null"},
    {"stdout_path", "stdout-path", "/dev/null"},
    {"stderr_path", "stderr-path", "/dev/null"},
    {"lifetime", "lifetime", "10"},
    {"max_mem", "cgroup-mem", "1024"},
    {"max_pid", "cgroup-pid", "30"},
    {"cpus", "cgroup-cpus", "1"},
    {"rlimit_stack", "rlimit-stack", "0"},
    {"rlimit_fsize", "rlimit-fsize", "128"},
    {"uid", "uid", nullptr},
    {"gid", "gid", nullptr},
    {"report_fd", "report-fd", "1"}
};

class lantern_call {
 private:
  vector<string> args;
  vector<const char *> args_str;
 public:
  explicit lantern_call(string bin) {
      args.push_back(std::move(bin));
  }
  void append(string s) {
      args.push_back(std::move(s));
  }
  const char **freeze() {
      args_str.clear();
      args_str.reserve(args.size() + 10);
      for (const auto &e : args) args_str.push_back(e.c_str());
      args_str.push_back(nullptr);
      return &args_str.front();
  }
};

lantern_call make_lantern_call(const httplib::Request &req, char *lantern_binary) {

    lantern_call lc{lantern_binary};

    for (const auto &[from, to, def]:latern_mapping) {
        auto value_got = req.get_param_value(from);
        if (value_got.empty() && def) value_got = def;
        if (!value_got.empty()) lc.append("--"s + to + "=" + value_got);
    }

    if (req.get_param_value("execve_once") == "false") lc.append("-o");

    lc.append("--");

    auto args_str = req.get_param_value("args_str");
    std::stringstream ss(args_str);
    string s, bin;
    if (!(ss >> bin)) throw std::invalid_argument("bin not found");
    lc.append(bin);
    while (ss >> s) lc.append(s);

    return lc;
}

string collect_stdout(const char *bin, char *const *args) {
    int pipe_fd[2];
    if (pipe2(pipe_fd, O_CLOEXEC)) {
        throw hsc_error(HscError::EPipe);
    }

    if (auto pid = fork(); pid < 0) {
        throw hsc_error(HscError::EChild);
    } else if (pid != 0) {
        close(pipe_fd[1]);
        defer _(nullptr, [=](...) { close(pipe_fd[0]); });
        char buf[4100];
        memset(buf, 0, sizeof(buf));
        string res;
        while (read(pipe_fd[0], buf, 4096) > 0) {
            res += buf;
        }
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (status) {
            auto exit_code = WEXITSTATUS(status);
            throw hsc_error(static_cast<HscError>(exit_code));
        } else {
            return res;
        }
    } else {
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        execvp(bin, args);
    }
}

