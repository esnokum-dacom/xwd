#pragma once
#include <xcb/xcb.h>
#include <vector>
#include <cstdint>

struct MonInfo {
    int16_t x, y;
    int16_t w, h;
};

std::vector<MonInfo> query_mons(xcb_connection_t *conn);


