mode ?= gst
ARCH ?= $(shell uname -i)
PYTHON ?= /usr/bin/python3
DOCKER_SRC_IMAGE ?= "p2ptech/cross-build:2023-02-21-raspios-bullseye-armhf-lite"

ifneq ("$(wildcard .git)","")
    COMMIT ?= $(or $(shell git rev-parse HEAD), local)
    VERSION ?= $(or $(shell $(PYTHON) ./version.py $(shell git show -s --format="%ct" $(shell git rev-parse HEAD)) $(shell git rev-parse --abbrev-ref HEAD)), 0.0.0)
    SOURCE_DATE_EPOCH ?= $(or $(shell git show -s --format="%ct" $(shell git rev-parse HEAD)), $(shell date "+%s"))
else
    COMMIT ?= local
    VERSION ?= 0.0.0
    SOURCE_DATE_EPOCH ?= $(shell date "+%s")
endif

export VERSION COMMIT SOURCE_DATE_EPOCH

CFLAGS += -DWFB_OSD_VERSION='"$(VERSION)-$(shell /bin/bash -c '_tmp=$(COMMIT); echo $${_tmp::8}')"'

ifeq ($(mode), gst)
    CFLAGS += -Wall -pthread -std=gnu99 -D__GST_OPENGL__ -fPIC $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
    LDFLAGS += $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gstreamer-1.0) $(shell pkg-config --libs gstreamer-video-1.0) -lgstapp-1.0 -lpthread -lrt -lm
    OBJS = main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o appsrc.o gst-compat.o
else ifeq ($(mode), rockchip)
    CFLAGS += -Wall -pthread -std=gnu99 -D__DRM_ROCKCHIP__ -fPIC $(shell pkg-config --cflags libdrm)
    LDFLAGS += $(shell pkg-config --libs libdrm) -lpthread -lrt -lm
    OBJS = main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o drm_output.o
else ifeq ($(mode), rpi3)
    CFLAGS += -Wall -pthread -std=gnu99 -D__BCM_OPENVG__ -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
    LDFLAGS += -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm
    OBJS = main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o oglinit.o
else
    $(error Valid modes are: gst, rockchip or rpi3)
endif

all: osd

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

osd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


osd_docker:  /opt/qemu/bin
	@if ! [ -d /opt/qemu ]; then echo "Docker cross build requires patched QEMU!\nApply ./docker/qemu.patch to qemu-7.2.0 and build it:\n  ./configure --prefix=/opt/qemu --static --disable-system && make && sudo make install"; exit 1; fi
	if ! ls /proc/sys/fs/binfmt_misc | grep -q qemu ; then sudo ./docker/qemu-binfmt-conf.sh --qemu-path /opt/qemu/bin --persistent yes; fi
	TAG="wfb-ng-osd:build-`date +%s`"; docker build -t $$TAG docker --build-arg SRC_IMAGE=$(DOCKER_SRC_IMAGE)  && \
	docker run -i --rm -v $(PWD):/build $$TAG bash -c "trap 'chown -R --reference=. .' EXIT; export VERSION=$(VERSION) COMMIT=$(COMMIT) SOURCE_DATE_EPOCH=$(SOURCE_DATE_EPOCH) && cd /build && make clean && make osd mode=$(mode)"
	docker image ls -q "wfb-ng-osd:build-*" | uniq | tail -n+6 | while read i ; do docker rmi -f $$i; done

clean:
	rm -f osd *.o *~
	make -C fpv_video clean

