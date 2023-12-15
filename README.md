This project started from https://github.com/TobiasBales/PlayuavOSD.git

Supported platforms:
-------------------
  * Raspberry Pi 0-3 -- native mode (use VideoCore IV API)
  * Linux with X11 -- use gstreamer overlay

Supported autopilots:
---------------------

   * PX4 -- full support
   * Ardupilot -- should work, but not tested
   * any mavlink-based -- should work with small fixes

Building:
---------

1. Build for Linux X11 (native build):
  * `apt-get install libcairo2-dev gstreamer1.0-tools libgstreamer1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly libgstreamer-plugins-base1.0-dev`
  * `make osd`

2. Build for Raspberry PI (cross-build):
  * `make build_rpi` (you need linux machine with docker installed)

3. Build for Raspberry PI (native build):
  * `make osd mode=rpi`

Running:
--------

Default mavlink port which OSD listen is 14550.

1. RPI: `./osd`
2. Linux X11:
   * Run test video stream (OSD will not work without video stream)
   ```
      gst-launch-1.0 videotestsrc pattern=black ! videoconvert ! \
      video/x-raw,format=NV12,framerate=30/1,width=1280,height=720,format=I420 ! \
      x264enc bitrate=4000 tune=zerolatency ! rtph264pay config-interval=1 ! \
      udpsink host=127.0.0.1 port=5600
   ```
   * Run OSD: `./osd`
   * You should got screen like this:
     ![gstreamer](scr1.png)


Screenshots:
------------
![px4](scr2.png)
