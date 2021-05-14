#pragma once

#include <iostream>
#include <seccomp.h>

#include "child.h"
#include "args_t.h"

#include <sys/resource.h>
#include <grp.h>

struct child_context {
  args_t args;
  std::shared_ptr<cgroup> cg;
  int *pipe_fd;

};

int child(void *params) {
    auto ctx = *(static_cast<child_context *>(params));
    auto &cg = ctx.cg;
    auto &args = ctx.args;
    auto pid = getpid();

    close(ctx.pipe_fd[0]);
    auto write_fd = ctx.pipe_fd[1];
    defer _(nullptr, [=](...) { close(write_fd); });

    try {
        /* redirect stdin */
        if (args.stdin_path) {
            if (auto fd = open(args.stdin_path, O_RDONLY); fd != -1) {
                defer _(nullptr, [=](...) { close(fd); });
                if (dup2(fd, STDIN_FILENO) == -1)throw hsc_error(HscError::EDup2Stdin);
            } else {
                throw hsc_error(HscError::EOpen);
            }
        }


        /* redirect stdout */
        if (args.stdout_path) {
            if (auto fd = open(args.stdout_path, O_WRONLY | O_CREAT | O_TRUNC, 0600); fd != -1) {
                defer _(nullptr, [=](...) { close(fd); });
                if (dup2(fd, STDOUT_FILENO) == -1)throw hsc_error(HscError::EDup2Stdout);
            } else {
                throw hsc_error(HscError::EOpen);
            }
        }

        /* redirect stderr */
        if (args.stderr_path) {
            if (auto fd = open(args.stderr_path, O_WRONLY | O_CREAT | O_TRUNC, 0600); fd != -1) {
                defer _(nullptr, [=](...) { close(fd); });
                if (dup2(fd, STDERR_FILENO) == -1)throw hsc_error(HscError::EDup2Stderr);
            } else {
                throw hsc_error(HscError::EOpen);
            }
        }

        if (args.cwd) {
            if (chdir(args.cwd)) {
                throw hsc_error(HscError::EChdir);
            }
        }

        if (args.execve_once) {
            scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
            if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 1,
                                 SCMP_A0(SCMP_CMP_NE, (scmp_datum_t)(args.pathname))) != 0) {
                seccomp_release(ctx);
                throw hsc_error(HscError::ESeccomp);
            }
            if (seccomp_load(ctx)) {
                throw hsc_error(HscError::ESeccompLoad);
            }
        }

        if (args.rlimit_fsize) {
            const rlimit rl = {.rlim_cur=args.rlimit_fsize, .rlim_max=args.rlimit_fsize};
            setrlimit(RLIMIT_FSIZE, &rl);
        }

        if (args.rlimit_stack) {
            const rlimit rls = {.rlim_cur=args.rlimit_stack, .rlim_max=args.rlimit_stack};
            setrlimit(RLIMIT_STACK, &rls);
        }

        if (args.gid) {
            gid_t groups[] = {args.gid};
            if (setgid(args.gid) || setgroups(1, groups)) {
                throw hsc_error(HscError::ESetGid);
            }
        }

        cg->attach_all(pid);

        if (args.uid) {
            if (setuid(args.uid)) {
                throw hsc_error(HscError::ESetUid);
            }
        }
    } catch (hsc_error &e) {
        write_to_fd(write_fd, static_cast<int>(e.error));
        return -1;
    } catch (...) {
        write_to_fd(write_fd, static_cast<int>(HscError::EUnknown));
        return -2;
    }

    if (execvp(args.pathname, args.args)) {
        write_to_fd(write_fd, static_cast<int>(HscError::EExec));
        return -3;
    }

    return 0;
}
