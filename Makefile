SUBDIRS := lib
SUBDIRS += xdp-sctp xdp-udp-reflector udp-client

TOPTARGETS := all install clean

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
