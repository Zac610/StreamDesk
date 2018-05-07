# StreamDesk
Play multimedia streams frameless on your desktop

![Alt text](screenshot.png?raw=true "Screenshot")

The actual code is in StreamDeskMain/StreamDesk.c

Other files are for CodeLite framework.

It is possible to compile the code with the command:
```
gcc StreamDesk.c -o StreamDesk `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-3.0 gstreamer-1.0`
```

The needed libraries can be installed with the command:
```
sudo apt install libgtk-3-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0 gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools
```
