#include <cstdlib>
#include <xcb/xinerama.h>
#include <cstdint>

#include "monitors.hpp"

std::vector<MonInfo> query_mons(xcb_connection_t *conn) {
    std::vector<MonInfo> mons;

    xcb_xinerama_is_active_cookie_t active_cookie = xcb_xinerama_is_active(conn);
    xcb_xinerama_is_active_reply_t *reply_cookie = xcb_xinerama_is_active_reply(conn, active_cookie, nullptr);

    bool active = reply_cookie && reply_cookie->state;
    free(reply_cookie);

    if (!active) return mons;

    xcb_xinerama_query_screens_cookie_t cookie = xcb_xinerama_query_screens(conn);
    xcb_xinerama_query_screens_reply_t *repl = xcb_xinerama_query_screens_reply(conn, cookie, nullptr);
    if (!repl) return mons;

    xcb_xinerama_screen_info_iterator_t iter = xcb_xinerama_query_screens_screen_info_iterator(repl);
    for (; iter.rem; xcb_xinerama_screen_info_next(&iter)) {
        MonInfo m;
        m.x = iter.data->x_org;
        m.y = iter.data->y_org;
        m.w = iter.data->width;
        m.h = iter.data->height;
        mons.push_back(m);
    }
    free(repl);
    return mons;
}
