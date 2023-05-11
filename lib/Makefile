

include defines.mk

LIB_DIR := .
LIB_INSTALL := $(LIB_DIR)/install

all: $(OBJECT_LIBBPF) $(BPFTOOL) $(OBJECT_LIBXDP)

.PHONY: clean
clean: libbpf_clean bpftool_clean libxdp_clean

install:
	install -m 0755 -d $(DESTDIR)$(SBINDIR)
	install -m 0755 -d $(DESTDIR)$(HDRDIR)
	$(MAKE) -C bpftool/src install
	$(MAKE) -C libxdp install
	$(MAKE) -C testing install


libbpf: $(OBJECT_LIBBPF)
bpftool: libbpf $(BPFTOOL)
libxdp: libbpf $(BPFTOOL) $(OBJECT_LIBXDP)

# Detect submodule libbpf source file changes
LIBBPF_SOURCES := $(wildcard libbpf/src/*.[ch])

$(OBJECT_LIBBPF): $(LIBBPF_SOURCES)
	@echo ; echo "  libbpf"
	$(MAKE) -C libbpf/src CFLAGS="$(LIBBPF_CFLAGS)"
	$(MAKE) -C libbpf/src DESTDIR=../../$(LIB_INSTALL) PREFIX= install_headers
	cp -fp libbpf/src/libbpf.a $(LIB_INSTALL)/lib/

.PHONY: libbpf_clean
libbpf_clean:
	$(MAKE) -C libbpf/src clean

# Detect submodule bpftool source file changes
BPFTOOL_SOURCES := $(wildcard bpftool/src/*.[ch])
$(BPFTOOL): $(BPFTOOL_SOURCES)
	@echo ; echo "  bpftool"
	$(MAKE) -C bpftool/src CFLAGS="$(LIBBPF_CFLAGS)"
	$(MAKE) -C bpftool/src DESTDIR=../../$(LIB_INSTALL) PREFIX= install
	cp -fp bpftool/src/bpftool $(LIB_INSTALL)/sbin

.PHONY: bpftool_clean
bpftool_clean:
	$(MAKE) -C bpftool/src clean

# Detect submodule libxdp source file changes
LIBXDP_SOURCES := $(wildcard xdp-tools/lib/libxdp/libxdp*.[ch]) xdp-tools/lib/libxdp/xsk.c

$(OBJECT_LIBXDP): $(LIBXDP_SOURCES)
	@echo ; echo "  libxdp"
	$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL=$(LIB_INSTALL)/sbin/bpftool libxdp
	$(MAKE) -C xdp-tools DESTDIR=../../../$(LIB_INSTALL) PREFIX= BUILD_STATIC_ONLY=1 libxdp_install

.PHONY: libxdp_clean
libxdp_clean:
	$(MAKE) -C xdp-tools clean