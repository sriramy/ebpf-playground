LIB_DIR := .
LIB_INSTALL := $(LIB_DIR)/install

include defines.mk

all: libbpf bpftool libxdp xdploader xdpbench xdptrafficgen common

.PHONY: clean
clean: libbpf_clean bpftool_clean libxdp_clean xdploader_clean xdpbench_clean xdptrafficgen_clean common_clean
	$(Q)find $(LIB_INSTALL) -type f -not -name .gitignore -delete
	$(Q)find $(LIB_INSTALL) -type d -empty -delete

libbpf: $(LIBBPF)
bpftool: libbpf $(BPFTOOL)
libxdp: bpftool $(LIBXDP)
xdploader: libxdp $(XDP_LOADER)
xdpbench: libxdp $(XDP_BENCH)
xdptrafficgen: libxdp $(XDP_TRAFFICGEN)
common: libxdp $(COMMON)

$(LIBBPF): $(LIBBPF_SOURCES)
	@echo "  libbpf"
	$(Q)$(MAKE) -C libbpf/src EXTRA_CFLAGS="-fPIC" $(P)
	$(Q)$(MAKE) -C libbpf/src DESTDIR=../../$(LIB_INSTALL) PREFIX= install_headers $(P)
	$(Q)cp -fp libbpf/src/libbpf.a $(LIB_INSTALL)/lib/ $(P)

.PHONY: libbpf_clean
libbpf_clean:
	$(Q)$(MAKE) -C libbpf/src clean
	$(Q)find $(LIB_INSTALL) -type f -name libbpf.a -delete

$(BPFTOOL): $(BPFTOOL_SOURCES)
	@echo "  bpftool"
	$(Q)$(MAKE) -C bpftool/src EXTRA_CFLAGS="--static" $(P)
	$(Q)cp -fp bpftool/src/bpftool $(LIB_INSTALL)/sbin $(P)

.PHONY: bpftool_clean
bpftool_clean:
	$(Q)$(MAKE) -C bpftool/src clean
	$(Q)find $(LIB_INSTALL) -type f -name bpftool -delete

$(LIBXDP): $(LIBXDP_SOURCES)
	@echo "  libxdp"
	$(Q)$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL="../$(LIB_INSTALL)/sbin/bpftool" LIBBPF_DIR="../$(LIB_DIR)/libbpf" LIBBPF_INCLUDE_DIR="../$(LIB_INSTALL)/install/include" LIBBPF_UNBUILT=1 libxdp $(P)
	$(Q)$(MAKE) -C xdp-tools DESTDIR=../../../$(LIB_INSTALL) PREFIX= BUILD_STATIC_ONLY=1 libxdp_install $(P)

.PHONY: libxdp_clean
libxdp_clean:
	$(Q)$(MAKE) -C xdp-tools clean
	$(Q)find $(LIB_INSTALL) -type f -name libxdp.a -delete

$(XDP_LOADER): $(XDP_LOADER_SOURCES)
	@echo "  xdp-loader"
	$(Q)$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL="../../../$(LIB_INSTALL)/sbin/bpftool" xdp-loader $(P)
	$(Q)cp -fp xdp-tools/xdp-loader/xdp-loader $(LIB_INSTALL)/bin/ $(P)

.PHONY: xdploader_clean
xdploader_clean:
	$(Q)find $(LIB_INSTALL) -type f -name xdp-loader -delete

$(XDP_BENCH): $(XDP_BENCH_SOURCES)
	@echo "  xdp-bench"
	$(Q)$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL="../../$(LIB_INSTALL)/sbin/bpftool" xdp-bench $(P)
	$(Q)cp -fp xdp-tools/xdp-bench/xdp-bench $(LIB_INSTALL)/bin/ $(P)

.PHONY: xdpbench_clean
xdpbench_clean:
	$(Q)find $(LIB_INSTALL) -type f -name xdp-bench -delete

$(XDP_TRAFFICGEN): $(XDP_TRAFFICGEN_SOURCES)
	@echo "  xdp-trafficgen"
	$(Q)$(MAKE) -C xdp-tools BUILD_STATIC_ONLY=1 BPFTOOL="../../$(LIB_INSTALL)/sbin/bpftool" xdp-trafficgen $(P)
	$(Q)cp -fp xdp-tools/xdp-trafficgen/xdp-trafficgen $(LIB_INSTALL)/bin/ $(P)

.PHONY: xdptrafficgen_clean
xdptrafficgen_clean:
	$(Q)find $(LIB_INSTALL) -type f -name xdp-trafficgen -delete

$(COMMON): $(COMMON_SOURCES)
	@echo "  common"
	$(Q)$(MAKE) -C common EXTRA_CFLAGS="-I../$(LIB_INSTALL)/include" $(P)
	$(Q)$(MAKE) -C common DESTDIR="../$(LIB_INSTALL)/" install $(P)

.PHONY: common_clean
common_clean:
	$(Q)$(MAKE) -C common clean $(P)
	$(Q)find $(LIB_INSTALL) -type f -name libcommon.a -delete

.PHONY: libdpdk
libdpdk: $(LIBDPDK_SOURCES)
	@echo "  libdpdk"
	$(Q)meson setup --prefix=$(abspath $(LIB_INSTALL)) dpdk/build dpdk $(P)
	$(Q)meson configure -Dmax_lcores=4 -Dexamples=all dpdk/build $(P)
	$(Q)meson compile -C dpdk/build $(P)
	$(Q)meson install -C dpdk/build $(P)

.PHONY: libdpdk_clean
libdpdk_clean:
	$(Q)ninja -C dpdk/build -t clean $(P)
	$(Q)rm -rf install/lib/x86_64-linux-gnu/ $(P)
	$(Q)rm -rf install/include/generic/ $(P)
	$(Q)rm -f install/include/rte* $(P)
	$(Q)rm -f install/include/cmdline* $(P)
	$(Q)find $(LIB_INSTALL) -type f -name dpdk-* -delete
	$(Q)find $(LIB_INSTALL) -type f -name librte* -delete

install: all
	$(Q)install -m 0755 -d $(DESTDIR)$(HDRDIR)/bpf $(P)
	$(Q)install -m 644 $(LIB_INSTALL)/include/bpf/* -t $(DESTDIR)$(HDRDIR)/bpf $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(HDRDIR)/xdp $(P)
	$(Q)install -m 644 $(LIB_INSTALL)/include/xdp/* -t $(DESTDIR)$(HDRDIR)/xdp $(P)
	$(Q)install -m 0755 -d $(DESTDIR)$(HDRDIR)/common $(P)
	$(Q)install -m 644 $(LIB_INSTALL)/include/common/* -t $(DESTDIR)$(HDRDIR)/common $(P)
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