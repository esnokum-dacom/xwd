#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string>

#include "xcb_context.hpp"
#include "wallpaper.hpp"
#include "config.hpp"
#include "ipc_server.hpp"

static std::string default_sock_path() {
    const char *xdg = std::getenv("XDG_RUNTIME_DIR");
    if (xdg) return std::string(xdg) + "/xwd.sock";
    return "/tmp/xwd-" + std::to_string(getuid()) + ".sock";
}

static void daemonize() {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); std::exit(1); }
    if (pid > 0) std::exit(0);

    if (setsid() < 0) { perror("setsid"); std::exit(1); }

    pid = fork();
    if (pid < 0) { perror("fork"); std::exit(1); }
    if (pid > 0) std::exit(0);

    umask(0);
    if (chdir("/") != 0) {}

    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }
}

int main(int argc, char **argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    bool foreground = false;
    std::vector<std::string> rest;
    for (auto &a : args) {
        if (a == "-f" || a == "--foreground") foreground = true;
        else rest.push_back(a);
    }

    if (!foreground) daemonize();

    std::string cfg_path = !rest.empty() ? rest[0] : Config::default_path();
    Config cfg = Config::load(cfg_path);

    try {
        XcbContext ctx;
        Wallpaper_t wp(ctx);

        if (!cfg.wallpapers.empty())
            wp.set(cfg.wallpapers[0], cfg.mode);

        IpcServer ipc(ctx, wp, cfg, default_sock_path());
        ipc.run();
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
