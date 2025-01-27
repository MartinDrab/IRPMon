
CFLAGS += -DNOVSOCKETS -D_WIN32_WINNT=0x0600
LDFLAGS += -shared -L../device-connector
LDLIBS += -ldevice-connector -lws2_32

EXE = libserver.dll
OBJS = libserver.o

include ../common.mk
