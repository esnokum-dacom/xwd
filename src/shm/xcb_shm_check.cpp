#include "xcb_shm_check.hpp"
#include <cstdlib>

bool shm_av(xcb_connection_t *conn) {
    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(conn, &xcb_shm_id);
    if (!ext || !ext->present) return false;

    xcb_shm_query_version_cookie_t cookie = xcb_shm_query_version(conn);
    xcb_shm_query_version_reply_t *reply = xcb_shm_query_version_reply(conn, cookie, nullptr);
    if (!reply) return false;

    bool ok = reply->shared_pixmaps;
    free(reply);
    return ok;
}
