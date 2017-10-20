CFLAGS += -g -Wall -std=gnu99 -I$(SYSROOT)/opt/vc/include/ -I$(SYSROOT)/opt/vc/include/interface/vcos/pthreads -I$(SYSROOT)/opt/vc/include/interface/vmcs_host/linux
LDFLAGS += -L$(SYSROOT)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm

all: osd

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

osd: main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o oglinit.o
	$(CC) -o $@ $^ $(LDFLAGS)

build_rpi: clean
	docker build docker -t wifibroadcast_osd:rpi_raspbian
	docker run -i -t --rm -v $(PWD):/build -v $(PWD):/rpxc/sysroot/build wifibroadcast_osd:rpi_raspbian sh -c 'CFLAGS=--sysroot=/rpxc/sysroot LDFLAGS="--sysroot=/rpxc/sysroot" CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc make'
	docker run -i -t --rm -v $(PWD):/build -v $(PWD):/rpxc/sysroot/build wifibroadcast_osd:rpi_raspbian sh -c 'cd /rpxc/sysroot/opt/vc/src/hello_pi/libs/ilclient; SDKSTAGE=/rpxc/sysroot CFLAGS=--sysroot=/rpxc/sysroot LDFLAGS="--sysroot=/rpxc/sysroot" CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc make; cd /build/fpv_video; SDKSTAGE=/rpxc/sysroot CFLAGS=--sysroot=/rpxc/sysroot LDFLAGS="--sysroot=/rpxc/sysroot" CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc make'
	mkdir -p dist
	tar czf dist/wifibroadcast_osd.tar.gz osd -C fpv_video fpv_video fpv_video.sh

clean:
	rm -f osd *.o *~
	make -C fpv_video clean

