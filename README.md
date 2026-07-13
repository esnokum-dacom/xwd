# XWallpaper Daemon
XWD is a wallpaper Daemon, is not different, has a daemon, and can set up your images and videos like mp4, mpv, gif, etc.

XWD is IPC controlled, can play pretty much anything that ffmpeg can. Extremely lightweight, depending only on X11 and ffmpeg.
## Dependencies
- xcb xcb-xinerama xcb-shm
- libavformat libavcodec libavutil libswscale
- Latest C++ compiler
- ffmpeg


# Build
Simply clone and run. Ensure CMake is installed
```
./build.sh
```
in the project directory. // home/user/etc/xwd.


# install
I have no plans to upload to any package manager; you can install by copying the repo and using
```
./install.sh
```

## Usage
First, start the daemon by running:
```
xwd
```

To set a live wallpaper you can put 
```
xwdctl SET <PATH (IMG/MP4/GIF)>
```

If you want to put in a monitor one live wallpaper you can with
```
xwdctl VIDEO <MON_INDEX> <PATH (IMG/MP4/GIF)>
```

To know the monitor index you can
```
xwdctl MONITORS
```

[Example video (I don't know what happened with the format)](https://github.com/esnokum-dacom/xwd/blob/main/output.mp4)
// Use mpv or something while I install OBS

# FEATURES
- Runtime control via IPC without ever needing to restart the daemon
- Can play videos, GIFs, and static images.
- Runtime pause and play commands

# Limitations
No Hardware acceleration. XCB is so powerful but still limited; if I research a little bit more, I can probably implement this for images, videos, gifs, etc. And improve the performance
