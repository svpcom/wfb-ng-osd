mode?=gst

ifeq ($(mode), gst)
CFLAGS += -Wall -pthread -std=gnu99 -D__GST_CAIRO__ -fPIC $(shell pkg-config --cflags cairo) $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gstreamer-1.0) $(shell pkg-config --libs cairo) $(shell pkg-config --libs gstreamer-video-1.0) -lpthread -lrt -lm
OBJS = main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o cairo_overlay.o
else
CFLAGS += -Wall -pthread -std=gnu99 -D__BCM_OPENVG__ -I$(SYSROOT)/opt/vc/include/ -I$(SYSROOT)/opt/vc/include/interface/vcos/pthreads -I$(SYSROOT)/opt/vc/include/interface/vmcs_host/linux
LDFLAGS += -L$(SYSROOT)/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm
OBJS = main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o oglinit.o
endif

all: osd

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

osd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f osd *.o *~
	make -C fpv_video clean

