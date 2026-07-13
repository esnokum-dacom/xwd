#include "wallpaper_v.hpp"
#include "xcb_pxf.hpp"
#include "shm/xcb_shm_check.hpp"

#include <xcb/shm.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <stdexcept>
#include <thread>
#include <chrono>

V_Wallpaper::~V_Wallpaper() {
    stop();
    if (decode_thread_.joinable()) decode_thread_.join();
    teardown_shm();
}

void V_Wallpaper::stop() {
    stop_flag_.store(true);
}

void V_Wallpaper::teardown_shm() {
    if (gc_) { xcb_free_gc(ctx_.conn(), gc_); gc_ = 0; }
    shm_.reset();
}

void V_Wallpaper::setup_shm(uint16_t w, uint16_t h) {
    if (!shm_av(ctx_.conn()))
        throw std::runtime_error("MIT-SHM not available on this X server");

    xcb_format_t *format = find_format(ctx_.conn(), ctx_.screen()->root_depth);
    if (!format) throw std::runtime_error("could not resolve pixel format");

    depth_ = ctx_.screen()->root_depth;
    uint32_t bpp = format->bits_per_pixel / 8;
    stride_ = ((w * bpp + format->scanline_pad / 8 - 1) / (format->scanline_pad / 8))
              * (format->scanline_pad / 8);
    w_ = w;
    h_ = h;

    shm_ = std::make_unique<ShmSeg>(ctx_.conn(), static_cast<size_t>(stride_) * h_);

    gc_ = xcb_generate_id(ctx_.conn());
    xcb_create_gc(ctx_.conn(), gc_, ctx_.root(), 0, nullptr);
}

void V_Wallpaper::decode_loop(std::string path, MonInfo mon, xcb_pixmap_t target_pixmap) {
    AVFormatContext *fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0) return;
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return;
    }

    AVCodec *decoder = nullptr;
    int stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (stream_idx < 0) {
        avformat_close_input(&fmt_ctx);
        return;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[stream_idx]->codecpar);
    if (avcodec_open2(codec_ctx, decoder, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return;
    }

    SwsContext *sws = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        w_, h_, AV_PIX_FMT_BGRA,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return;
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    AVRational tb = fmt_ctx->streams[stream_idx]->time_base;

    uint8_t *dst_data[4] = { nullptr, nullptr, nullptr, nullptr };
    int dst_linesize[4] = { static_cast<int>(stride_), 0, 0, 0 };

    using clock = std::chrono::steady_clock;
    auto playback_start = clock::now();
    int64_t first_pts = AV_NOPTS_VALUE;

    while (!stop_flag_.load()) {
        int read_ret = av_read_frame(fmt_ctx, packet);
        if (read_ret < 0) {
            av_seek_frame(fmt_ctx, stream_idx, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codec_ctx);
            playback_start = clock::now();
            first_pts = AV_NOPTS_VALUE;
            continue;
        }

        if (packet->stream_index != stream_idx) {
            av_packet_unref(packet);
            continue;
        }

        if (avcodec_send_packet(codec_ctx, packet) == 0) {
            while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                int64_t pts = frame->best_effort_timestamp;
                if (pts != AV_NOPTS_VALUE) {
                    if (first_pts == AV_NOPTS_VALUE) first_pts = pts;
                    double target_sec = (pts - first_pts) * av_q2d(tb);
                    auto target_time = playback_start + std::chrono::duration<double>(target_sec);
                    std::this_thread::sleep_until(target_time);
                }

                if (stop_flag_.load()) break;

                dst_data[0] = shm_->data();
                sws_scale(sws, frame->data, frame->linesize, 0, codec_ctx->height,
                          dst_data, dst_linesize);

                xcb_shm_put_image(ctx_.conn(), target_pixmap, gc_,
                                   w_, h_, 0, 0, w_, h_,
                                   mon.x, mon.y, depth_,
                                   XCB_IMAGE_FORMAT_Z_PIXMAP, 0,
                                   shm_->xid(), 0);
                xcb_clear_area(ctx_.conn(), 1, ctx_.root(), mon.x, mon.y, w_, h_);
                xcb_flush(ctx_.conn());
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    sws_freeContext(sws);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

void V_Wallpaper::play(const std::string &path, MonInfo mon, xcb_pixmap_t target_pixmap) {
    stop_flag_.store(false);

    xcb_visualtype_t *vis = find_vtype(ctx_.screen());
    if (!vis) throw std::runtime_error("could not resolve visual");
    if (vis->red_mask != 0x00ff0000 || vis->green_mask != 0x0000ff00 || vis->blue_mask != 0x000000ff)
        throw std::runtime_error("unsupported visual layout for video path (expected 0xRRGGBB masks)");

    uint8_t byte_or = xcb_get_setup(ctx_.conn())->image_byte_order;
    if (byte_or != XCB_IMAGE_ORDER_LSB_FIRST)
        throw std::runtime_error("unsupported byte order for video path (expected LSB-first)");

    setup_shm(static_cast<uint16_t>(mon.w), static_cast<uint16_t>(mon.h));

    decode_thread_ = std::thread(&V_Wallpaper::decode_loop, this, path, mon, target_pixmap);
}
