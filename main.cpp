#include <iostream>
#include "args_t.h"
#include "cgroup.h"
#include "child.h"

constexpr auto stack_size = 4096 * 1024;

char stack[stack_size];
char *stack_top = stack + stack_size;

void exec(const args_t &args) {
    auto cg = std::make_shared<cgroup>(std::to_string(random_key()));

    cg->attach_controller("memory");
    cg->attach_controller("pids");
    cg->attach_controller("cpuacct");
    if (args.cgroup_pids) cg->set_value("pids", "pids.max", args.cgroup_pids);
    cg->set_value("memory", "memory.limit_in_bytes", args.cgroup_mem);

    defer _(nullptr, [=](...) { cg->destroy(); });

    int pipe_fd[2];
    if (pipe2(pipe_fd, O_CLOEXEC)) {
        throw hsc_error(HscError::EPipe);
    }
    child_context ctx{args, cg, pipe_fd};
    auto child_pid = clone(child, stack_top, clone_flag, &ctx);
    if (child_pid == -1) throw hsc_error(HscError::EChild);

    auto read_fd = pipe_fd[0];
    close(pipe_fd[1]);
    defer _c(nullptr, [=](...) { close(pipe_fd[0]); });

    if (auto lifetime = args.lifetime;lifetime) {
        std::thread([=]() {
          std::this_thread::sleep_for(std::chrono::seconds(lifetime));
          kill(child_pid, SIGKILL);
        }).detach();
    }

    auto err = read_from_fd<int>(read_fd);
    if (err.has_value()) {
        throw hsc_error(static_cast<HscError>(err.value()));
    }

    int status;
    if (waitpid(child_pid, &status, WUNTRACED) == -1 && errno != ECHILD) {
        throw hsc_error(HscError::EWait);
    }

    std::tuple t = {
        WEXITSTATUS(status),
        WTERMSIG(status),
        cg->get_value<uint64_t>("cpuacct", "cpuacct.usage_sys"),
        cg->get_value<uint64_t>("cpuacct", "cpuacct.usage_user"),
        cg->get_value<uint64_t>("memory", "memory.max_usage_in_bytes")
    };

    auto &out = args.fd == 2 ? std::cerr : std::cout;

    std::apply([&out](auto &&... args) { ((..., (out << args << ' '))); }, std::move(t));
    out << std::endl;
}

int main(int argc, char **argv) {
    auto args = args_t::make(argc, argv);

    try {
        exec(args);
    } catch (hsc_error &e) {
        std::cout << static_cast<int>(e.error) << ' ' << e.err << std::endl;
    } catch (std::exception &e) {
        std::cout << static_cast<int>(HscError::EStdException) << ' ' << e.what() << std::endl;
    } catch (...) {
        std::cout << static_cast<int>(HscError::EUnknown) << ' ' << errno << std::endl;
    }
    return 0;
}


