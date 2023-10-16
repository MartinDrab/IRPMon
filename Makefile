
dlls = \
    libcallbackstreamdll \
    libdparserdll \
    libreqlistdll \
    libsymbolsdll \
    irpmondll

main = irpmonc

.PHONY: all clean $(dlls) $(main)

all: $(dlls)
	$(MAKE) -C $(main)

$(dlls):
	$(MAKE) -C $@

clean:
	$(foreach dir,$(dlls) $(main),$(MAKE) -C $(dir) $@ &&) :
