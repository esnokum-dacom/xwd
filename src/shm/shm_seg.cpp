#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdexcept>

#include "shm_seg.hpp"

ShmSeg::ShmSeg(xcb_connection_t *conn, size_t size) : conn_(conn), size_(size) {
    shmid_ = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    if (shmid_ < 0) throw std::runtime_error("shmget failed");

    data_ = static_cast<uint8_t*>(shmat(shmid_, nullptr, 0));
    if (data_ == reinterpret_cast<void*>(-1)) {
        shmctl(shmid_, IPC_RMID, nullptr);
        throw std::runtime_error("shmat failed");
    }

    seg_ = xcb_generate_id(conn_);
    xcb_void_cookie_t cookie = xcb_shm_attach_checked(conn_, seg_, shmid_, 0);
    xcb_generic_error_t *err = xcb_request_check(conn_, cookie);
    if (err) {
        free(err);
        shmdt(data_);
        shmctl(shmid_, IPC_RMID, nullptr);
        throw std::runtime_error("xcb_shm_attach failed");
    }

    shmctl(shmid_, IPC_RMID, nullptr);
}

ShmSeg::~ShmSeg() {
    xcb_shm_detach(conn_, seg_);
    if (data_) shmdt(data_);
}
