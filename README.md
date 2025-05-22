This project started from https://github.com/TobiasBales/PlayuavOSD.git

Supported platforms:
-------------------
  * Raspberry Pi 0-3 -- use hardware overlay mode (OpenVG)
  * Radxa Zero 3W/3E -- use hardware overlay mode (libdrm)
  * OrangePi 5 -- use hardware overlay mode (libdrm)
  * Any other Linux with X11/Wayland and GPU -- use GStreamer OpenGL mixer

Supported autopilots:
---------------------

   * PX4 -- full support
   * Ardupilot -- should work, but not tested
   * any mavlink-based -- should work with small fixes

Building:
---------

1. Build for Linux (X11 or Wayland) (native build):
  * `apt-get install gstreamer1.0-tools pkg-config libgstreamer1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly libgstreamer-plugins-base1.0-dev`
  * `make osd`

2. Build for Raspberry PI 0-3 (OpenVG) (native build):
  * `make osd mode=rpi3`

3. Build for Radxa or OrangePi (libdrm) (native build):
  * `apt-get install libdrm-dev pkg-config`
  * `make osd mode=rockchip`

Running:
--------

Default mavlink port is UDP 14551.
Default RTP video port is UDP 5600.

   * Run `./osd`
   * You should got screen like this:
     ![gstreamer](scr1.png)


Screenshots:
------------
![px4](scr2.png)

Wiki: [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/svpcom/wfb-ng-osd)
------------
