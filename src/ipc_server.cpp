#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "ipc_server.hpp"
#include "config.hpp"
#include "wallpaper_v.hpp"
#include "monitors.hpp"

namespace {

bool is_video_path(const std::string &path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;

    std::string ext = path.substr(dot + 1);
    for (auto &c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    static const std::unordered_set<std::string> video_exts = {
        "mp4", "m4v", "mkv", "webm", "mov", "avi", "flv", "wmv",
        "mpg", "mpeg", "ts", "m2ts", "ogv", "gif"
    };
    return video_exts.count(ext) > 0;
}

} // namespace

IpcServer::IpcServer(XcbContext &ctx, Wallpaper_t &wp, Config &cfg, std::string sock_path)
    : ctx_(ctx), wp_(wp), cfg_(cfg), sock_path_(std::move(sock_path)) {
    setup_socket();
}

IpcServer::~IpcServer() {
    stop_video(-1);
    if (listen_fd_ >= 0) close(listen_fd_);
    unlink(sock_path_.c_str());
}

std::vector<MonInfo> IpcServer::monitors() {
    std::vector<MonInfo> mons = query_mons(ctx_.conn());
    if (mons.empty()) {
        mons.push_back({0, 0, static_cast<int16_t>(ctx_.screen()->width_in_pixels),
                         static_cast<int16_t>(ctx_.screen()->height_in_pixels)});
    }
    return mons;
}

void IpcServer::stop_video(int mon_idx) {
    if (mon_idx < 0) {
        video_wps_.clear();
        video_paths_.clear();
        return;
    }
    video_wps_.erase(mon_idx);
    video_paths_.erase(mon_idx);
}

void IpcServer::start_video(int mon_idx, const std::string &path) {
    std::vector<MonInfo> mons = monitors();
    if (mon_idx < 0 || static_cast<size_t>(mon_idx) >= mons.size())
        throw std::runtime_error("no such monitor index: " + std::to_string(mon_idx));

    video_wps_.erase(mon_idx);

    auto vw = std::make_unique<V_Wallpaper>(ctx_);
    vw->play(path, mons[mon_idx], wp_.pixmap());
    video_wps_[mon_idx] = std::move(vw);
    video_paths_[mon_idx] = path;
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
        std::string path = line.substr(4);
        try {
            stop_video(-1);

            if (is_video_path(path)) {
                wp_.ensure_pixmap();
                std::vector<MonInfo> mons = monitors();
                for (size_t i = 0; i < mons.size(); ++i)
                    start_video(static_cast<int>(i), path);
            } else {
                wp_.set(path, cfg_.mode);
            }
            return "OK\n";
        } catch (const std::exception &e) {
            return std::string("ERR ") + e.what() + "\n";
        }
    }
    if (line.rfind("VIDEO ", 0) == 0) {
        std::istringstream iss(line.substr(6));
        int mon_idx;
        if (!(iss >> mon_idx))
            return "ERR usage: VIDEO <monitor-index> <path>\n";
        std::string path;
        std::getline(iss, path);
        size_t a = path.find_first_not_of(' ');
        if (a == std::string::npos) return "ERR usage: VIDEO <monitor-index> <path>\n";
        path = path.substr(a);

        try {
            start_video(mon_idx, path);
            return "OK\n";
        } catch (const std::exception &e) {
            return std::string("ERR ") + e.what() + "\n";
        }
    }
    if (line == "STOP") {
        stop_video(-1);
        return "OK\n";
    }
    if (line.rfind("STOP ", 0) == 0) {
        try {
            stop_video(std::stoi(line.substr(5)));
            return "OK\n";
        } catch (const std::exception &e) {
            return std::string("ERR ") + e.what() + "\n";
        }
    }
    if (line == "MONITORS") {
        std::vector<MonInfo> mons = monitors();
        std::ostringstream oss;
        for (size_t i = 0; i < mons.size(); ++i)
            oss << i << ": " << mons[i].w << "x" << mons[i].h
                << "+" << mons[i].x << "+" << mons[i].y << "\n";
        return oss.str();
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
