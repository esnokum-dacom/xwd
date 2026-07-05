#include "xcb_pxf.hpp"
#include <xcb/xcb.h>
#include <xcb/xproto.h>

xcb_visualtype_t *find_vtype(xcb_screen_t *scr) {
    xcb_depth_iterator_t depth_i = xcb_screen_allowed_depths_iterator(scr);
    for (; depth_i.rem; xcb_depth_next(&depth_i)) {
	xcb_visualtype_iterator_t visual_t = xcb_depth_visuals_iterator(depth_i.data);
	for (; visual_t.rem; xcb_visualtype_next(&visual_t)) {
	    if (scr->root_visual == visual_t.data->visual_id)
		return visual_t.data;
	}
    }
    return nullptr;
}

xcb_format_t *find_format(xcb_connection_t *conn, uint8_t depth) {
    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_format_iterator_t fmt_i = xcb_setup_pixmap_formats_iterator(setup);
    
    for (; fmt_i.rem; xcb_format_next(&fmt_i)) {
	if (fmt_i.data->depth == depth)
	    return fmt_i.data;
    }
    return nullptr;
}
