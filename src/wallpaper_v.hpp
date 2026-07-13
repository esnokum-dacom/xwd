#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

#include "xcb_context.hpp"
#include "monitors.hpp"
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

    void play(const std::string &path, MonInfo mon, xcb_pixmap_t target_pixmap);
    void stop();

private:
    XcbContext &ctx_;
    std::unique_ptr<ShmSeg> shm_;
    xcb_gcontext_t gc_ = 0;
    uint32_t stride_ = 0;
    uint16_t w_ = 0, h_ = 0;
    uint8_t depth_ = 0;
    std::atomic<bool> stop_flag_{false};
    std::thread decode_thread_;

    void setup_shm(uint16_t w, uint16_t h);
    void teardown_shm();
    void decode_loop(std::string path, MonInfo mon, xcb_pixmap_t target_pixmap);
};
