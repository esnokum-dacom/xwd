#pragma once
#include "xcb_context.hpp"
#include "wallpaper.hpp"
#include "wallpaper_v.hpp"
#include "config.hpp"
#include <string>
#include <memory>
#include <thread>

class IpcServer {
public:
    IpcServer(XcbContext &ctx, Wallpaper_t &wp, Config &cfg, std::string sock_path);
    ~IpcServer();

    IpcServer(const IpcServer&) = delete;
    IpcServer& operator=(const IpcServer&) = delete;

    void run();

private:
    XcbContext &ctx_;
    Wallpaper_t &wp_;
    Config &cfg_;
    std::string sock_path_;
    int listen_fd_ = -1;
    size_t current_idx_ = 0;

    std::unique_ptr<V_Wallpaper> video_wp_;
    std::thread video_thread_;

    void setup_socket();
    void handle_client(int fd);
    std::string dispatch(const std::string &line);
    void apply(size_t idx);
    void stop_video();
    void start_video(const std::string &path);
};
