#pragma once
#include <string>
#include <vector>

enum class ScaleMode { Stretch, Fill };

struct Config {
    std::vector<std::string> wallpapers;
    ScaleMode mode = ScaleMode::Fill;

    static Config load(const std::string &path);
    static std::string default_path();
};

ScaleMode ParseMode(const std::string &s);
