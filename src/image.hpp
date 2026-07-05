#pragma once 
#include <sys/types.h>
#include <vector>
#include <cstdint>
#include <string>

struct ImgBuf {
    std::vector<uint8_t> pxs;
    uint16_t w = 0;
    uint16_t h = 0;
    uint8_t channels = 3;
};

ImgBuf load_img(const std::string &path);
ImgBuf scale_near(const ImgBuf &src, uint16_t dst_w, uint16_t dst_h);
ImgBuf scale_fill(const ImgBuf &src, uint16_t dst_w, uint16_t dst_h);
