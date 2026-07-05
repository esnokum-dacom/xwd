#pragma once
#include <cstdint>
#include <xcb/xcb.h>

xcb_visualtype_t *find_vtype(xcb_screen_t *scr);
xcb_format_t *find_format(xcb_connection_t *conn, uint8_t depth);
