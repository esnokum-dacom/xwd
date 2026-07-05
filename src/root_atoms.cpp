#include <cstdlib>
#include <stdexcept>
#include <string>
#include <xcb/xproto.h>

#include "root_atoms.hpp"

static xcb_atom_t intern(xcb_connection_t *conn, const std::string &name) {
    auto cookie = xcb_intern_atom(conn, 0, name.size(), name.c_str());
    xcb_intern_atom_reply_t *repl = xcb_intern_atom_reply(conn, cookie, nullptr);

    if (!repl) throw std::runtime_error("Intern Atom failed - " + name);
    xcb_atom_t atom = repl->atom;
    free(repl);
    return atom;
}

void set_root_pmap(xcb_connection_t *conn, xcb_window_t root, xcb_pixmap_t pmap) {
    xcb_atom_t root_pamp_id = intern(conn, "_XROOTPMAP_ID");
    xcb_atom_t esetroot_id = intern(conn, "ESETROOT_PMAP_ID");

    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root, root_pamp_id, XCB_ATOM_PIXMAP, 32, 1, &pmap);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root, esetroot_id, XCB_ATOM_PIXMAP, 32, 1, &pmap);
}
