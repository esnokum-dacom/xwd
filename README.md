# XWallpaper Daemon
XWD is a wallpaper Daemon, is not different, have a daemon and can setup your images and videos like mp4, mpv, gif, etc.

XWD is IPC controlled, can play pretty much anything that ffmpeg can. extremely lightweight, depending only on X11 and ffmpeg.
## Dependencies
- xcb xcb-xinerama xcb-shm
- libavformat libavcodec libavutil libswscale
- Latest c++ compiler
- ffmpeg


# Build
Simply clone and run. Ensure CMake is installed
```
./build.sh
```
in the project directory. // home/user/etc/xwd.


# install
I have no planned to up in any package manager, you can install copying the repo and using
```
./install.sh
```

## Usage
First start the daemon by running:
```
xwd
```

Then control it with xwdctl
```
xwdctl -h | --help
```



# FEATURES
- Runtime control via IPC without ever needing to restart the daemon
- Can play videos, gifs, and static images.
- Runtime pause and play commands

# TO DO 
- Hardware acceleration.
- Multi monitor support (I'm not sure if it works properly in more than 1 monitor)

# Limitations
No Hardware acceleration. XCB are so powerfull but still being limitated, if I research a little bit more, probably, I can implement this for images, videos, gifs, etc. And improve the performance
