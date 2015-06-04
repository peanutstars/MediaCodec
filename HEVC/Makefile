
CC		= gcc

FFMPEG_LD_PATH	= /work/ffmpeg/codec/ffmpeg/

CFLAGS	+= -Wall -g
CFLAGS	+= -I$(FFMPEG_LD_PATH)
LDFLAGS += -L$(FFMPEG_LD_PATH)/libavcodec -L$(FFMPEG_LD_PATH)/libavdevice -L$(FFMPEG_LD_PATH)/libavfilter -L$(FFMPEG_LD_PATH)/libavformat -L$(FFMPEG_LD_PATH)/libavresample -L$(FFMPEG_LD_PATH)/libavutil -L$(FFMPEG_LD_PATH)/libpostproc -L$(FFMPEG_LD_PATH)/libswscale -L$(FFMPEG_LD_PATH)/libswresample -L/home/hslee/opt/ffmpeg/lib  -Wl,--as-needed -Wl,-z,noexecstack -Wl,--warn-common -Wl,-rpath-link=libpostproc:libswresample:libswscale:libavfilter:libavdevice:libavformat:libavcodec:libavutil:libavresample -lavdevice -lavfilter -lavformat -lavcodec -lpostproc -lswresample -lswscale -lavutil -lva -lva-x11 -lva -lxcb -lX11 -ljack -lnettle -lhogweed -lgmp -L/home/hslee/opt/ffmpeg/lib -lx264 -lpthread -lm -ldl -lfontconfig -lgnutls -ldl -lm -lz -pthread -lrt -ldl  -lvdpau

TARGET	= hevc_sei

OBJS	= hevc_sei.o

all:	$(TARGET)

$(TARGET):	$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(TARGET) *.o
