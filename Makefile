
depdirs = \
    libcallbackstreamdll \
    libdparserdll \
    libreqlistdll \
    libsymbolsdll \
    irpmondll \
    libserver

maindirs = irpmonc irpmon-server

otherdirs = device-connector shared

.PHONY: all clean $(depdirs) $(maindirs) $(otherdirs)

all: $(otherdirs) $(maindirs)

$(maindirs): $(depdirs)
	$(MAKE) -C $@

$(depdirs) $(otherdirs):
	$(MAKE) -C $@

libserver: device-connector

clean:
	$(foreach dir,$(depdirs) $(maindirs) $(otherdirs),$(MAKE) -C $(dir) $@ &&) :
