CPPFLAGS+= -g -Wall -std=gnu99 -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
LDFLAGS+= -lfreetype -lz
LDFLAGS+=-L/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -lshapes

all: osd

%.o: %.c
	gcc -c -o $@ $< $(CPPFLAGS)

osd: main.o osdrender.o osdmavlink.o graphengine.o UAVObj.o m2dlib.o math3d.o osdconfig.o osdvar.o fonts.o font_outlined8x14.o font_outlined8x8.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -f osd *.o *~

