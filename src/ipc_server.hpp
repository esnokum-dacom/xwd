#pragma once
#include "xcb_context.hpp"
#include "wallpaper.hpp"
#include "wallpaper_v.hpp"
#include "config.hpp"
#include "monitors.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

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
    std::unordered_map<int, std::unique_ptr<V_Wallpaper>> video_wps_;
    std::unordered_map<int, std::string> video_paths_;

    void setup_socket();
    void handle_client(int fd);
    std::string dispatch(const std::string &line);
    void apply(size_t idx);
    void stop_video(int mon_idx);
    void start_video(int mon_idx, const std::string &path);
    std::vector<MonInfo> monitors();
};
