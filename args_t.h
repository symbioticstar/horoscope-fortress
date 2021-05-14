#pragma once

#include "hsc.h"

#include <cstdint>
#include <argp.h>
#include <cassert>

const char *argp_program_version = "1.0.0";
const char *argp_program_bug_address = "<i@sst.st>";
static constexpr char doc[] = "Horoscope Fortress";
static constexpr char args_doc[] = " -- [BIN] [ARGS]";

enum option_key {
  rlimit_fsize = 'f',
  rlimit_stack = 's',
  uid = 'u',
  gid = 'g',
  disable_execve_once = 'o',
  report_fd = 'r',
  lifetime = 't',
  cgroup_mem = 'm',
  cgroup_pids = 'p',
  stdin_path = 1000,
  stdout_path = 1001,
  stderr_path = 1002,
  cwd = 'c'
};

static constexpr argp_option options[] = {
    {"rlimit-fsize", option_key::rlimit_fsize, "MiB"},
    {"rlimit-stack", option_key::rlimit_stack, "MiB"},
    {"lifetime", option_key::lifetime, "s", 0},
    {"cgroup-memory", option_key::cgroup_mem, "MiB"},
    {"cgroup-pids", option_key::cgroup_pids, "count"},

    {"stdin-path", option_key::stdin_path, "path"},
    {"stdout-path", option_key::stdout_path, "path"},
    {"stderr-path", option_key::stderr_path, "path"},

    {"uid", option_key::uid, "uid"},
    {"gid", option_key::gid, "gid"},
    {"report-fd", option_key::report_fd, "fd"},
    {
        "disable-execve-once", option_key::disable_execve_once, nullptr, OPTION_ARG_OPTIONAL,
        "Allow execve call with pathname provided only"
    },
    {nullptr}
};

class args_t {
  static int parse_opt(int key, char *arg, struct argp_state *state);

 public:

  uint64_t rlimit_fsize{128 << 20};
  uint64_t rlimit_stack{0};
  bool execve_once{true};
  bool stderr_to_stdout{false};

  uid_t uid{0};
  gid_t gid{0};
  uint32_t fd{1};

  uint32_t lifetime{0};
  uint64_t cgroup_mem{1 << 30};
  uint64_t cgroup_pids{10};

  char *stdin_path{nullptr};
  char *stdout_path{nullptr};
  char *stderr_path{nullptr};
  char *cwd{nullptr};

  char *pathname{};
  char **args{};

  args_t() = default;

  static args_t make(int argc, char **argv);
};

int args_t::parse_opt(int key, char *arg, struct argp_state *state) {
    auto *self = static_cast<args_t *>(state->input);
    option_key s;

    switch (key) {
        case ::rlimit_fsize:self->rlimit_fsize = strtoull(arg, nullptr, 10) << 20;
            break;
        case ::rlimit_stack:self->rlimit_stack = strtoull(arg, nullptr, 10) << 20;
            break;
        case ::uid:self->uid = strtoul(arg, nullptr, 10);
            break;
        case ::gid:self->gid = strtoul(arg, nullptr, 10);
            break;
        case ::report_fd
            :self->fd = strtoul(arg, nullptr, 10);
            break;
        case ::disable_execve_once:self->execve_once = false;
            break;
        case ::lifetime:self->lifetime = strtoul(arg, nullptr, 10);
            break;
        case ::cgroup_mem:self->cgroup_mem = strtoull(arg, nullptr, 10) << 20;
            break;
        case ::cgroup_pids:self->cgroup_pids = strtoull(arg, nullptr, 10) << 20;
            break;
        case ::stdin_path:self->stdin_path = arg;
            break;
        case ::stdout_path:self->stdout_path = arg;
            break;
        case ::stderr_path:self->stderr_path = arg;
            break;
        case ::cwd:self->cwd = arg;
            break;
        case ARGP_KEY_NO_ARGS:argp_usage(state);
            break;
        case ARGP_KEY_ARG:self->pathname = arg;
            self->args = &state->argv[state->next - 1];
            state->next = state->argc;
            break;
        default:return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

args_t args_t::make(int argc, char **argv) {
    args_t self;
    struct argp argp = {options, parse_opt, args_doc, doc};
    argp_parse(&argp, argc, argv, 0, nullptr, &self);

    return self;
}
