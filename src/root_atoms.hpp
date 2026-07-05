#pragma once
#include <xcb/xcb.h>

void set_root_pmap(xcb_connection_t *conn, xcb_window_t root, xcb_pixmap_t pmap);
