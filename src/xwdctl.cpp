#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

int main(int argc, char **argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    if (argc < 2) {
        fprintf(stderr, "usage: xwdctl <SET|VIDEO path|NEXT|PREV|MODE fill|stretch>\n");
        return 1;
    }

    for (auto &a : args) {
	if (a == "-h" || a == "--help") {
	    printf("usage: xwdctl <SET|VIDEO> path \n");
	    printf("Config: xwdctl <PREV|NEXT> setted in the config file\n");
	    return 0;
	}
    }

    std::string cmd = argv[1];
    for (int i = 2; i < argc; ++i) cmd += std::string(" ") + argv[i];

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    const char *xdg = getenv("XDG_RUNTIME_DIR");
    std::string sock_path = xdg ? std::string(xdg) + "/xwd.sock"
                                 : "/tmp/xwd-" + std::to_string(getuid()) + ".sock";
    std::strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    cmd += "\n";
    write(fd, cmd.c_str(), cmd.size());

    char buf[512];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) { buf[n] = '\0'; fputs(buf, stdout); }

    close(fd);
    return 0;
}
