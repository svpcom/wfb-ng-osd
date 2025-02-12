ARG SRC_IMAGE
FROM $SRC_IMAGE

ARG QEMU_CPU
ENV QEMU_CPU=$QEMU_CPU

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive \
    apt-get install -y lsb-release build-essential git wget \
    python3-all python3-all-dev python3-setuptools \
    virtualenv dh-python fakeroot debhelper \
    libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev \
    libdrm-dev pkg-config

RUN git clone -b oldstable --depth 1 https://github.com/raspberrypi/firmware /tmp/firmware && mv /tmp/firmware/opt/vc /opt && rm -rf /tmp/firmware

COPY src/Makefile /tmp
ENV ENV=/opt/env
ENV PYTHON=python3
RUN cd /tmp && make $ENV
