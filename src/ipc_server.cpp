#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <stdexcept>
#include <vector>

#include "ipc_server.hpp"
#include "config.hpp"
#include "wallpaper_v.hpp"

IpcServer::IpcServer(XcbContext &ctx, Wallpaper_t &wp, Config &cfg, std::string sock_path)
    : ctx_(ctx), wp_(wp), cfg_(cfg), sock_path_(std::move(sock_path)) {
    setup_socket();
}

IpcServer::~IpcServer() {
    stop_video();
    if (listen_fd_ >= 0) close(listen_fd_);
    unlink(sock_path_.c_str());
}

void IpcServer::stop_video() {
    if (video_wp_) {
        video_wp_->stop();
    }
    if (video_thread_.joinable()) {
        video_thread_.join();
    }
    video_wp_.reset();
}

void IpcServer::start_video(const std::string &path) {
    stop_video();

    video_wp_ = std::make_unique<V_Wallpaper>(ctx_);
    V_Wallpaper *raw = video_wp_.get();
    video_thread_ = std::thread([raw, path]() {
        raw->play(path);
    });
}

void IpcServer::setup_socket() {
    listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw std::runtime_error("socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (sock_path_.size() >= sizeof(addr.sun_path))
        throw std::runtime_error("socket path too long: " + sock_path_);
    std::strncpy(addr.sun_path, sock_path_.c_str(), sizeof(addr.sun_path) - 1);

    unlink(sock_path_.c_str());

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed on " + sock_path_);

    if (listen(listen_fd_, 5) < 0)
        throw std::runtime_error("listen() failed");
}

void IpcServer::apply(size_t idx) {
    if (cfg_.wallpapers.empty()) return;
    idx = idx % cfg_.wallpapers.size();
    wp_.set(cfg_.wallpapers[idx], cfg_.mode);
    current_idx_ = idx;
}

std::string IpcServer::dispatch(const std::string &line) {
    if (line.rfind("SET ", 0) == 0) {
        try {
            wp_.set(line.substr(4), cfg_.mode);
            return "OK\n";
        } catch (const std::exception &e) {
            return std::string("ERR ") + e.what() + "\n";
        }
    }
    if (line.rfind("VIDEO ", 0) == 0) {
	try {
	    start_video(line.substr(6));
	    return "Ok, Started " + line.substr(6) + "\n";
	} catch (const std::exception &e) {
	    return std::string("ERR ") + e.what() + "\n";
	}
    }
    if (line == "STOP" || line.rfind("STOP ", 0) == 0) {
	stop_video();
	return "Stopped\n";
    }
    if (line == "NEXT") {
        if (cfg_.wallpapers.empty()) return "ERR no wallpapers configured\n";
        apply(current_idx_ + 1);
        return "OK\n";
    }
    if (line == "PREV") {
        if (cfg_.wallpapers.empty()) return "ERR no wallpapers configured\n";
        apply(current_idx_ + cfg_.wallpapers.size() - 1);
        return "OK\n";
    }
    if (line.rfind("MODE ", 0) == 0) {
        cfg_.mode = ParseMode(line.substr(5));
        return "OK\n";
    }
    return "ERR unknown command\n";
}

void IpcServer::handle_client(int fd) {
    char buf[512];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) { close(fd); return; }
    buf[n] = '\0';

    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        line.pop_back();

    std::string response = dispatch(line);
    write(fd, response.c_str(), response.size());
    close(fd);
}

void IpcServer::run() {
    std::vector<pollfd> fds = {
        { listen_fd_, POLLIN, 0 },
        { ctx_.fd(), POLLIN, 0 }
    };

    while (true) {
        int ret = poll(fds.data(), fds.size(), -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll() failed");
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(listen_fd_, nullptr, nullptr);
            if (client_fd >= 0) handle_client(client_fd);
        }

        if (fds[1].revents & POLLIN) {
            xcb_generic_event_t *ev;
            while ((ev = xcb_poll_for_event(ctx_.conn())) != nullptr)
                free(ev);
        }
    }
}
