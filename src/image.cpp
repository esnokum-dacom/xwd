#include "../external-dependencies/stb_image.h"
#include <stdexcept>
#include <algorithm>

#include "image.hpp"


ImgBuf load_img(const std::string &path) {
    int width, height, channels;

    uint8_t *data = stbi_load(path.c_str(), &width, &height, &channels, 3);
    if (!data) throw std::runtime_error("stbi_load failed: " + path);

    ImgBuf img;
    img.w = static_cast<uint16_t>(width);
    img.h = static_cast<uint16_t>(height);
    img.channels = 3;
    img.pxs.assign(data, data + (width * height * 3));

    stbi_image_free(data);

    return img;
}

ImgBuf scale_near(const ImgBuf &src, uint16_t dst_w, uint16_t dst_h) {
    ImgBuf dst;
    dst.w = dst_w;
    dst.h = dst_h;
    dst.channels = src.channels;
    dst.pxs.resize(dst_w * dst_h * src.channels);

    for (int i = 0; i < dst_h; ++i) {
        int sy = i * src.h / dst_h;
        for (int j = 0; j < dst_w; ++j) {
            int sx = j * src.w / dst_w;
            const uint8_t *s = &src.pxs[(sy * src.w + sx) * src.channels];
            uint8_t *d = &dst.pxs[(i * dst_w + j) * dst.channels];
            for (int k = 0; k < src.channels; ++k) d[k] = s[k];
        }
    }
    return dst;
}

static ImgBuf crop_center(const ImgBuf &src, uint16_t out_w, uint16_t out_h) {
    ImgBuf dst;
    dst.w = out_w;
    dst.h = out_h;
    dst.channels = src.channels;
    dst.pxs.resize(out_w * out_h * src.channels);

    int off_x = (src.w - out_w) / 2;
    int off_y = (src.h - out_h) / 2;

    for (int i = 0; i < out_h; ++i) {
        const uint8_t *srow = &src.pxs[((i + off_y) * src.w + off_x) * src.channels];
        uint8_t *drow = &dst.pxs[i * out_w * src.channels];
        std::copy(srow, srow + out_w * src.channels, drow);
    }
    return dst;
}

ImgBuf scale_fill(const ImgBuf &src, uint16_t dst_w, uint16_t dst_h) {
    float scale = std::max(static_cast<float>(dst_w) / src.w, static_cast<float>(dst_h) / src.h);

    uint16_t n_w = static_cast<uint16_t>(src.w * scale + 0.5f);
    uint16_t n_h = static_cast<uint16_t>(src.h * scale + 0.5f);

    ImgBuf scaled = scale_near(src , n_w, n_h);
    return crop_center(scaled, dst_w, dst_h);
}
