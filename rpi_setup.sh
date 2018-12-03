#!/bin/bash
set -e
set -x

apt-get update
apt-get install libraspberrypi0 libraspberrypi-bin gstreamer1.0-tools gstreamer1.0-plugins-good

cat > /lib/systemd/system/osd.service <<EOF
[Unit]
Description=FPV OSD
After=network-online.target

[Service]
Environment=LD_LIBRARY_PATH=/opt/vc/lib
ExecStart=/usr/bin/osd
Type=simple
Restart=always
RestartSec=1s
TimeoutStopSec=10s
KillMode=control-group

[Install]
WantedBy=multi-user.target
EOF

cat > /lib/systemd/system/fpv-video.service <<EOF
[Unit]
Description=FPV video
After=network-online.target

[Service]
ExecStart=/usr/bin/fpv_video.sh
Type=simple
Restart=always
RestartSec=1s
TimeoutStopSec=10s
KillMode=control-group

[Install]
WantedBy=multi-user.target
EOF

tar xzvf wifibroadcast_osd.tar.gz -C /usr/bin
(cd /opt/vc/lib; ln -sf libbrcmEGL.so libEGL.so; ln -sf libbrcmGLESv2.so libGLESv2.so)

systemctl daemon-reload
systemctl enable fpv-video
systemctl enable osd
systemctl restart fpv-video
systemctl restart osd
