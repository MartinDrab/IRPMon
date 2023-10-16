
CPPFLAGS += -DNOVSOCKETS

LDFLAGS += \
	   -L../irpmondll -lirpmondll \
	   -L../libdparserdll -llibdparser \
	   -L../libreqlistdll -llibreqlist \
	   -L../libsymbolsdll -llibsymbols \
	   -L../libcallbackstreamdll -lcallbackstream

EXE = irpmonc
OBJS = driver-settings.o irpmonc.o guid-api.o stop-event.o
DELOBJS = $(EXE).exe

include ../common.mk
