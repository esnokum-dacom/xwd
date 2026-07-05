#pragma once
#include <xcb/xcb.h>
#include <stdexcept>
#include <cstdint>

class XcbContext {
public:
    XcbContext() {
        conn_ = xcb_connect(nullptr, &screen_num_);
        if (xcb_connection_has_error(conn_))
            throw std::runtime_error("xcb_connect failed");

        const xcb_setup_t* setup = xcb_get_setup(conn_);
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
        for (int i = 0; i < screen_num_; ++i)
            xcb_screen_next(&iter);
        screen_ = iter.data;
    }

    ~XcbContext() {
        if (conn_) xcb_disconnect(conn_);
    }

    XcbContext(const XcbContext&) = delete;
    XcbContext& operator=(const XcbContext&) = delete;

    xcb_connection_t* conn() const { return conn_; }
    xcb_screen_t* screen() const { return screen_; }
    xcb_window_t root() const { return screen_->root; }
    int fd() const { return xcb_get_file_descriptor(conn_); }

private:
    xcb_connection_t* conn_ = nullptr;
    xcb_screen_t* screen_ = nullptr;
    int screen_num_ = 0;
};
