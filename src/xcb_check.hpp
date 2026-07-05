#pragma once
#include <cstdlib>
#include <xcb/xcb.h>
#include <cstdio>

inline void check_cookie(xcb_connection_t* conn, xcb_void_cookie_t cookie, const char* what) {
    xcb_generic_error_t* err = xcb_request_check(conn, cookie);
    if (err) {
        std::fprintf(stderr, "%s failed: error_code=%d major=%d minor=%d\n",
                     what, err->error_code, err->major_code, err->minor_code);
	free(err);
    }
}
