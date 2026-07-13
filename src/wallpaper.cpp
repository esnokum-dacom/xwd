#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "wallpaper.hpp"
#include "config.hpp"
#include "image.hpp"
#include "monitors.hpp"
#include "root_atoms.hpp"
#include "xcb_context.hpp"
#include "xcb_pxf.hpp"
#include "xcb_check.hpp"

static std::vector<u_int8_t> pack_pxs(const ImgBuf &img, xcb_visualtype_t *vis, xcb_format_t *format, uint8_t byte_or, uint32_t stride) {
    uint16_t red_shift = __builtin_ctz(vis->red_mask);
    uint16_t green_shift = __builtin_ctz(vis->green_mask);
    uint16_t blue_shift = __builtin_ctz(vis->blue_mask);
    uint32_t byte_ppx = format->bits_per_pixel / 8;

    std::vector<uint8_t> out(stride * img.h, 0);
    
    for (int i = 0; i < img.h; ++i){
	uint8_t *row = out.data() + i * stride;
	for (int j = 0; j < img.w; ++j){
	    const uint8_t* px = &img.pxs[(i * img.w + j) * img.channels];
            int pixel = (px[0] << red_shift) | (px[1] << green_shift) | (px[2] << blue_shift);
            uint8_t* dst = row + j * byte_ppx;

            if (byte_or == XCB_IMAGE_ORDER_LSB_FIRST) {
                dst[0] = pixel & 0xff;
                dst[1] = (pixel >> 8) & 0xff;
                dst[2] = (pixel >> 16) & 0xff;
                if (byte_or == 4) dst[3] = (pixel >> 24) & 0xff;
            } else {
                int i = 0;
                if (byte_ppx == 4) dst[i++] = (pixel >> 24) & 0xff;
                dst[i++] = (pixel >> 16) & 0xff;
                dst[i++] = (pixel >> 8) & 0xff;
                dst[i] = pixel & 0xff;
	    }
	}
    }
    return out;
}

static void put_imgch(xcb_connection_t *conn, xcb_pixmap_t pmap, xcb_gcontext_t gc, const std::vector<uint8_t> &buf, uint16_t w, uint16_t h, uint32_t stride, uint8_t depth, int16_t dst_x, int16_t dst_y) {
    uint32_t max_reqby = xcb_get_maximum_request_length(conn) * 4;
    uint32_t rows_pch = static_cast<uint32_t>((max_reqby - sizeof(xcb_put_image_request_t)) / stride);
    if (rows_pch < 1) rows_pch = 1;

    for (int i = 0; i < h; i += rows_pch) {
        uint32_t rows = std::min<uint16_t>(rows_pch, h - i);
        xcb_void_cookie_t cookie = xcb_put_image_checked(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, pmap, gc,
                  w, rows, dst_x, dst_y + i, 0, depth,
                  rows * stride, buf.data() + i * stride);
        check_cookie(conn, cookie, "put_image");
    }
}

xcb_atom_t Wallpaper_t::inter_at(const std::string &name) {
    auto cookie = xcb_intern_atom(ctx_.conn(), 0, name.size(), name.c_str());
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(ctx_.conn(), cookie, nullptr);

    if (!reply) throw std::runtime_error("Inter atom failed" + name);

    xcb_atom_t atom = reply->atom;

    free(reply);
    return atom;
}

void Wallpaper_t::set_root_at(xcb_pixmap_t pmap) {
    set_root_pmap(ctx_.conn(), ctx_.root(), pmap);
}

void Wallpaper_t::ensure_pixmap() {
    if (current_pix) return;

    uint32_t w = ctx_.screen()->width_in_pixels;
    uint32_t h = ctx_.screen()->height_in_pixels;

    xcb_pixmap_t pmap = xcb_generate_id(ctx_.conn());
    xcb_void_cookie_t pmap_cookie = xcb_create_pixmap_checked(
        ctx_.conn(), ctx_.screen()->root_depth, pmap, ctx_.root(), w, h);
    check_cookie(ctx_.conn(), pmap_cookie, "create_pixmap");

    xcb_gcontext_t gc = xcb_generate_id(ctx_.conn());
    uint32_t black = ctx_.screen()->black_pixel;
    xcb_void_cookie_t gc_cookie = xcb_create_gc_checked(
        ctx_.conn(), gc, pmap, XCB_GC_FOREGROUND, &black);
    check_cookie(ctx_.conn(), gc_cookie, "create_gc");

    xcb_rectangle_t rect = {0, 0, static_cast<uint16_t>(w), static_cast<uint16_t>(h)};
    xcb_poly_fill_rectangle(ctx_.conn(), pmap, gc, 1, &rect);
    xcb_free_gc(ctx_.conn(), gc);

    xcb_change_window_attributes(ctx_.conn(), ctx_.root(), XCB_CW_BACK_PIXMAP, &pmap);
    xcb_clear_area(ctx_.conn(), 0, ctx_.root(), 0, 0, w, h);
    set_root_at(pmap);
    xcb_flush(ctx_.conn());

    current_pix = pmap;
}

void Wallpaper_t::set(const std::string &path, ScaleMode mode) {
    ImgBuf img = load_img(path);

    uint32_t w = ctx_.screen()->width_in_pixels;
    uint32_t h = ctx_.screen()->height_in_pixels;

    xcb_visualtype_t *vis = find_vtype(ctx_.screen());
    xcb_format_t *format = find_format(ctx_.conn(), ctx_.screen()->root_depth);
    if (!vis || !format)  throw std::runtime_error("Could not resolve the pixel format");

    uint8_t byte_or = xcb_get_setup(ctx_.conn())->image_byte_order;
    uint8_t byte_ppx = format->bits_per_pixel / 8;

    xcb_pixmap_t pmap = xcb_generate_id(ctx_.conn());
    xcb_void_cookie_t pmap_cookie = xcb_create_pixmap_checked(
        ctx_.conn(), ctx_.screen()->root_depth, pmap, ctx_.root(), w, h);
    check_cookie(ctx_.conn(), pmap_cookie, "create_pixmap");

    xcb_gcontext_t gc = xcb_generate_id(ctx_.conn());
    xcb_void_cookie_t gc_cookie = xcb_create_gc_checked(ctx_.conn(), gc, pmap, 0, nullptr);
    check_cookie(ctx_.conn(), gc_cookie, "create_gc");

    std::vector<MonInfo> monitors = query_mons(ctx_.conn());
    if (monitors.empty())
        monitors.push_back({0, 0, static_cast<int16_t>(w), static_cast<int16_t>(h)});

    for (const auto &mon : monitors) {
        ImgBuf scaled = (mode == ScaleMode::Stretch)
            ? scale_near(img, mon.w, mon.h)
            : scale_fill(img, mon.w, mon.h);

        uint32_t mon_stride = ((mon.w * byte_ppx + format->scanline_pad / 8 - 1)
                                / (format->scanline_pad / 8) * (format->scanline_pad / 8));

        std::vector<uint8_t> packed = pack_pxs(scaled, vis, format, byte_or, mon_stride);
        put_imgch(ctx_.conn(), pmap, gc, packed, mon.w, mon.h, mon_stride,
                  ctx_.screen()->root_depth, mon.x, mon.y);
    }

    xcb_free_gc(ctx_.conn(), gc);

    xcb_change_window_attributes(ctx_.conn(), ctx_.root(), XCB_CW_BACK_PIXMAP, &pmap);
    xcb_clear_area(ctx_.conn(), 0, ctx_.root(), 0, 0, w, h);
    set_root_at(pmap);
    xcb_flush(ctx_.conn());

    if (current_pix) xcb_free_pixmap(ctx_.conn(), current_pix);
    current_pix = pmap;
}

void Wallpaper_t::set_region(const std::string &path, MonInfo mon, ScaleMode mode) {
    if (!current_pix) throw std::runtime_error("set_region called before ensure_pixmap");

    ImgBuf img = load_img(path);

    xcb_visualtype_t *vis = find_vtype(ctx_.screen());
    xcb_format_t *format = find_format(ctx_.conn(), ctx_.screen()->root_depth);
    if (!vis || !format) throw std::runtime_error("Could not resolve the pixel format");

    uint8_t byte_or = xcb_get_setup(ctx_.conn())->image_byte_order;
    uint8_t byte_ppx = format->bits_per_pixel / 8;

    xcb_gcontext_t gc = xcb_generate_id(ctx_.conn());
    xcb_void_cookie_t gc_cookie = xcb_create_gc_checked(ctx_.conn(), gc, current_pix, 0, nullptr);
    check_cookie(ctx_.conn(), gc_cookie, "create_gc");

    ImgBuf scaled = (mode == ScaleMode::Stretch)
        ? scale_near(img, mon.w, mon.h)
        : scale_fill(img, mon.w, mon.h);

    uint32_t mon_stride = ((mon.w * byte_ppx + format->scanline_pad / 8 - 1)
                            / (format->scanline_pad / 8) * (format->scanline_pad / 8));

    std::vector<uint8_t> packed = pack_pxs(scaled, vis, format, byte_or, mon_stride);
    put_imgch(ctx_.conn(), current_pix, gc, packed, mon.w, mon.h, mon_stride,
              ctx_.screen()->root_depth, mon.x, mon.y);

    xcb_free_gc(ctx_.conn(), gc);

    xcb_clear_area(ctx_.conn(), 1, ctx_.root(), mon.x, mon.y, mon.w, mon.h);
    xcb_flush(ctx_.conn());
}
