# StreamDesk
Play multimedia streams frameless on your desktop

![Alt text](screenshot.png?raw=true "Screenshot")

The actual code is in StreamDeskMain/StreamDesk.c

Other files are for CodeLite framework.

It is possible to compile the code with the command:
```
gcc StreamDesk.c -o StreamDesk `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-3.0 gstreamer-1.0`
```
