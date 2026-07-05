#include <cstddef>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "config.hpp"

std::string Config::default_path() {
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return std::string(xdg) + "/xwd/config";

    const char *home = std::getenv("HOME");
    if (home && *home) return std::string(home) + "/.config/xwd/config";

    return "/etc/xwd/config"; 
}

ScaleMode ParseMode(const std::string &s) {
    if (s == "stretch") return ScaleMode::Stretch;

    return ScaleMode::Fill;
}

static std::string trim (const std::string &s) {
    size_t a = s.find_first_not_of(" \t");
    size_t b = s.find_last_not_of(" \t");

    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

Config Config::load(const std::string &path) {
    Config cfg;
    std::ifstream f(path);
    if (!f) return cfg;

    std::string line;
    while (std::getline(f, line)) {
	line = trim(line);
	if (line.empty() || line[0] == '#') continue;

	size_t eq = line.find('=');
	if (eq == std::string::npos) continue;

	std::string key = trim(line.substr(0, eq));
	std::string val = trim(line.substr(eq + 1));

	if (key == "mode") cfg.mode = ParseMode(val);
	else if (key == "wallpaper") cfg.wallpapers.push_back(val);
    }
    return cfg;
}
