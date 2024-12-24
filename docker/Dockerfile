ARG SRC_IMAGE

FROM $SRC_IMAGE

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive \
    apt-get install -y build-essential git wget \
    virtualenv fakeroot debhelper \
    libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev \
    libdrm-dev pkg-config
