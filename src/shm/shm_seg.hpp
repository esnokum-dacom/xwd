#pragma once
#include <xcb/xcb.h>
#include <xcb/shm.h>

#include <cstddef>

class ShmSeg {
    public:
	ShmSeg(xcb_connection_t *conn, size_t sz);
	~ShmSeg();

	ShmSeg(const ShmSeg &) = delete;
	ShmSeg &operator=(const ShmSeg &) = delete;

	uint8_t *data() const { return data_; }
	size_t size() const { return size_; }
	xcb_shm_seg_t xid() const { return seg_; }

    private:
	xcb_connection_t *conn_;
	int shmid_ = -1;
	uint8_t *data_ = nullptr;
	size_t size_;
	xcb_shm_seg_t seg_ = 0;
};
