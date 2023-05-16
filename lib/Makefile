LIB_DIR := .
LIB_INSTALL := $(LIB_DIR)/install

include defines.mk

all: libbpf bpftool libxdp xdploader common

.PHONY: clean
clean: libbpf_clean bpftool_clean libxdp_clean xdploader_clean common_clean
	$(Q)find $(LIB_INSTALL) -type f -not -name .gitignore -delete
	$(Q)find $(LIB_INSTALL) -type d -empty -delete

libbpf: $(LIBBPF)
bpftool: libbpf $(BPFTOOL)
libxdp: bpftool $(LIBXDP)
xdploader: libxdp $(XDP_LOADER)
common: libxdp $(COMMON)

# Detect submodule libbpf source file changes
LIBBPF_SOURCES := $(wildcard libbpf/src/*.[ch])

$(LIBBPF): $(LIBBPF_SOURCES)
	@echo ; echo "  libbpf"
	$(Q)$(MAKE) -C libbpf/src CFLAGS="$(LIBBPF_CFLAGS)" $(P)
	$(Q)$(MAKE) -C libbpf/src DESTDIR=../../$(LIB_INSTALL) PREFIX= install_headers $(P)
	$(Q)cp -fp libbpf/src/libbpf.a $(LIB_INSTALL)/lib/ $(P)

.PHONY: libbpf_clean
libbpf_clean:
	$(Q)$(MAKE) -C libbpf/src clean
	$(Q)find $(LIB_INSTALL) -type f -name libbpf.a -delete

# Detect submodule bpftool source file changes
BPFTOOL_SOURCES := $(wildcard bpftool/src/*.[ch])
$(BPFTOOL): $(BPFTOOL_SOURCES)
	@echo ; echo "  bpftool"
	$(Q)$(MAKE) -C bpftool/src EXTRA_CFLAGS="--static" $(P)
	$(Q)cp -fp bpftool/src/bpftool $(LIB_INSTALL)/sbin $(P)

.PHONY: bpftool_clean
bpftool_clean:
	$(Q)$(MAKE) -C bpftool/src clean
	$(Q)find $(LIB_INSTALL) -type f -name bpftool -delete

# Detect submodule libxdp source file changes
LIBXDP_SOURCES := $(wildcard xdp-tools/lib/libxdp/libxdp*.[ch]) xdp-tools/lib/libxdp/xsk.c

$(LIBXDP): $(LIBXDP_SOURCES)
	@echo ; echo "  libxdp"
	$(Q)$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL="../$(LIB_INSTALL)/sbin/bpftool" LIBBPF_DIR="../$(LIB_DIR)/libbpf" LIBBPF_INCLUDE_DIR="../$(LIB_INSTALL)/install/include" LIBBPF_UNBUILT=1 libxdp $(P)
	$(Q)$(MAKE) -C xdp-tools DESTDIR=../../../$(LIB_INSTALL) PREFIX= BUILD_STATIC_ONLY=1 libxdp_install $(P)

.PHONY: libxdp_clean
libxdp_clean:
	$(Q)$(MAKE) -C xdp-tools clean
	$(Q)find $(LIB_INSTALL) -type f -name libxdp.a -delete

# Detect submodule xdp-loader source file changes
XDP_LOADER_SOURCES := $(wildcard xdp-tools/xdp-loader*.[ch]) 
$(XDP_LOADER): $(XDP_LOADER_SOURCES)
	@echo ; echo "  xdp-loader"
	$(Q)$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL="../../../$(LIB_INSTALL)/sbin/bpftool" xdp-loader $(P)
	$(Q)cp -fp xdp-tools/xdp-loader/xdp-loader $(LIB_INSTALL)/bin/ $(P)

.PHONY: xdploader_clean
xdploader_clean:
	$(Q)find $(LIB_INSTALL) -type f -name xdp-loader -delete

$(COMMON):
	@echo ; echo "  common"
	$(Q)$(MAKE) -C common CFLAGS="-I../$(LIB_INSTALL)/include" $(P)
	$(Q)cp -fp common/libcommon.a $(LIB_INSTALL)/lib

.PHONY: common_clean
common_clean:
	$(Q)$(MAKE) -C common clean $(P)
	$(Q)find $(LIB_INSTALL) -type f -name libcommon.a -delete

install: all
	$(Q)install -m 0755 -d $(DESTDIR)$(HDRDIR)/bpf $(P)
	$(Q)install -m 644 $(LIB_INSTALL)/include/bpf/* -t $(DESTDIR)$(HDRDIR)/bpf $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(HDRDIR)/xdp $(P)
	$(Q)install -m 644 $(LIB_INSTALL)/include/xdp/* -t $(DESTDIR)$(HDRDIR)/xdp $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(LIBDIR) $(P)
	$(Q)install -m 755 $(LIB_INSTALL)/lib/libbpf.a -t $(DESTDIR)$(LIBDIR) $(P)
	$(Q)install -m 755 $(LIB_INSTALL)/lib/libxdp.a -t $(DESTDIR)$(LIBDIR) $(P)
	$(Q)install -m 755 $(LIB_INSTALL)/lib/libcommon.a -t $(DESTDIR)$(LIBDIR) $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(LIBDIR)/bpf $(P)
	$(Q)install -m 755 $(LIB_INSTALL)/lib/bpf/* -t $(DESTDIR)$(LIBDIR)/bpf $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(BINDIR) $(P)
	$(Q)install -m 755 $(LIB_INSTALL)/bin/* -t $(DESTDIR)$(BINDIR) $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(SBINDIR) $(P)
	$(Q)install -m 755 $(LIB_INSTALL)/sbin/* -t $(DESTDIR)$(SBINDIR) $(P)