mode ?= gst
PYTHON ?= python3
SHELL = /bin/bash
ENV ?= $(PWD)/env
OS_CODENAME ?= $(shell lsb_release -cs)
STDEB ?= "git+https://github.com/svpcom/stdeb"
DOCKER_ARCH ?= amd64
DOCKER_SRC_IMAGE ?= "p2ptech/cross-build:2023-02-21-raspios-bullseye-armhf-lite"
QEMU_CPU ?= "max"

ifneq ("$(wildcard .git)","")
    RELEASE ?= $(or $(shell git rev-parse --abbrev-ref HEAD | grep -v '^stable$$'),\
                    $(shell git describe --all --match 'release-*' --match 'origin/release-*' --abbrev=0 HEAD 2>/dev/null | grep -o '[^/]*$$'),\
                    unknown)
    COMMIT ?= $(shell git rev-parse HEAD)
    SOURCE_DATE_EPOCH ?= $(or $(shell git show -s --format="%ct" $(COMMIT)), $(shell date "+%s"))
    VERSION ?= $(shell $(PYTHON) ./version.py $(SOURCE_DATE_EPOCH) $(RELEASE))
else
    COMMIT ?= release
    SOURCE_DATE_EPOCH ?= $(shell date "+%s")
    VERSION ?= $(or $(shell basename $(PWD) | grep -E -o '[0-9]+.[0-9]+(.[0-9]+)?$$'), 0.0.0)
endif

export VERSION COMMIT SOURCE_DATE_EPOCH
export OSD_MODE=$(mode)

$(ENV):
	$(PYTHON) -m virtualenv --download $(ENV)
	$$(PATH=$(ENV)/bin:$(ENV)/local/bin:$(PATH) which python3) -m pip install --upgrade pip setuptools $(STDEB)

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

osd: osd.$(mode)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

osd.$(mode): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

deb: osd $(ENV)
	rm -rf deb_dist
	$$(PATH=$(ENV)/bin:$(ENV)/local/bin:$(PATH) which python3) ./setup.py --command-packages=stdeb.command sdist_dsc --debian-version 0~$(OS_CODENAME) --package3 wfb-ng-osd-$(mode) bdist_deb
	rm -rf wfb_ng_osd.egg-info/ wfb-ng-osd-$(VERSION).tar.gz

deb_docker:  /opt/qemu/bin
	@if ! [ -d /opt/qemu ]; then echo "Docker cross build requires patched QEMU!\nApply ./docker/qemu.patch to qemu-7.2.0 and build it:\n  ./configure --prefix=/opt/qemu --static --disable-system && make && sudo make install"; exit 1; fi
	if ! ls /proc/sys/fs/binfmt_misc | grep -q qemu ; then sudo ./docker/qemu-binfmt-conf.sh --qemu-path /opt/qemu/bin --persistent yes; fi
	cp -a Makefile docker/src/
	TAG="wfb-ng-osd:build-`date +%s`"; docker build --platform linux/$(DOCKER_ARCH) -t $$TAG docker --build-arg SRC_IMAGE=$(DOCKER_SRC_IMAGE) --build-arg QEMU_CPU=$(QEMU_CPU)  && \
	docker run -i --rm --platform linux/$(DOCKER_ARCH) -v $(PWD):/build $$TAG bash -c "trap 'chown -R --reference=. .' EXIT; export VERSION=$(VERSION) COMMIT=$(COMMIT) SOURCE_DATE_EPOCH=$(SOURCE_DATE_EPOCH) && cd /build && make clean && make deb mode=$(mode)"
	docker image ls -q "wfb-ng-osd:build-*" | uniq | tail -n+6 | while read i ; do docker rmi -f $$i; done

osd_docker: deb_docker

clean:
	rm -rf osd.{rockchip,gst,rpi3} deb_dist *.o *~
	make -C fpv_video clean

