#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

#include "xcb_context.hpp"
#include "shm/shm_seg.hpp"

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;

class V_Wallpaper {
public:
    explicit V_Wallpaper(XcbContext &ctx) : ctx_(ctx) {}
    ~V_Wallpaper();

    V_Wallpaper(const V_Wallpaper&) = delete;
    V_Wallpaper& operator=(const V_Wallpaper&) = delete;

    void play(const std::string &path);
    void stop();

private:
    XcbContext &ctx_;
    std::unique_ptr<ShmSeg> shm_;
    xcb_pixmap_t pmap_ = 0;
    xcb_gcontext_t gc_ = 0;
    uint32_t stride_ = 0;
    uint16_t w_ = 0, h_ = 0;
    uint8_t depth_ = 0;
    std::atomic<bool> stop_flag_{false};

    std::mutex frame_mtx_;
    std::vector<uint8_t> pending_frame_;
    bool frame_ready_ = false;
    std::thread decode_thread_;
    std::thread render_thread_;

    void setup_shm_pixmap(uint16_t w, uint16_t h);
    void teardown_pixmap();
    void decode_loop(std::string path);
};
