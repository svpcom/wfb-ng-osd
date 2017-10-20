#!/bin/bash
gst-launch-1.0 udpsrc port=5600 caps='application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264' ! rtph264depay ! 'video/x-h264,stream-format=byte-stream' ! fdsink | ./fpv_video
