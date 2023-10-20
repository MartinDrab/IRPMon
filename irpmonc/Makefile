
CPPFLAGS += -DNOVSOCKETS

LDFLAGS += -municode \
   -L../irpmondll -L../libdparserdll -L../libreqlistdll \
   -L../libsymbolsdll -L../libcallbackstreamdll

LDLIBS += \
   -lirpmondll -llibdparser -llibreqlist \
   -llibsymbols -lcallbackstream

EXE = irpmonc
OBJS = driver-settings.o irpmonc.o guid-api.o stop-event.o
DELOBJS = $(EXE).exe

include ../common.mk
