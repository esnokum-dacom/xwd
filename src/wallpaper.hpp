#pragma once
#include <string>

#include "config.hpp"
#include "xcb_context.hpp"

class Wallpaper_t {
    public:
	explicit Wallpaper_t(XcbContext &ctx) : ctx_(ctx) {}
	void set(const std::string &path, ScaleMode mode = ScaleMode::Fill);

	xcb_pixmap_t pixmap() const { return current_pix; }

	void ensure_pixmap();

    private:
	XcbContext &ctx_;
	xcb_pixmap_t current_pix = 0;

	xcb_atom_t inter_at(const std::string &name);
	void set_root_at(xcb_pixmap_t pmap);
};
