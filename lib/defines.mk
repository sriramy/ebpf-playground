##
## Common definitions needed to build/install an xdp application
##

ifeq ("$(origin V)", "command line")
  VERBOSE = $(V)
endif
ifndef VERBOSE
  VERBOSE = 0
endif
ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif
ifeq ($(VERBOSE),1)
P = 
else
P = >/dev/null
MAKEFLAGS += --no-print-directory
endif

LLC ?= llc
CLANG ?= clang
CC ?= gcc
AR ?= ar

PREFIX?=/usr/local
LIBDIR?=$(PREFIX)/lib
BINDIR?=$(PREFIX)/bin
SBINDIR?=$(PREFIX)/sbin
HDRDIR?=$(PREFIX)/include/xdp
DATADIR?=$(PREFIX)/share
MANDIR?=$(DATADIR)/man
BPF_OBJECT_DIR ?=$(LIBDIR)/bpf

CFLAGS ?= -O2 -g -Werror -Wall
USER_CLFAGS ?= -O2 -g -Werror -Wall
BPF_CFLAGS ?= -Wno-visibility

DEFINES := -DBPF_OBJECT_PATH=\"$(BPF_OBJECT_DIR)\"

ifeq ($(DEBUG),1)
DEFINES += -ggdb -DDEBUG
endif

USER_CFLAGS += $(DEFINES)
BPF_CFLAGS += $(DEFINES)

LIB_DIR ?= lib
LIB_INSTALL := $(LIB_DIR)/install
LIBBPF_DIR := $(LIB_DIR)/libbpf
XDP_TOOLS := $(LIB_DIR)/xdp-tools

BUILD := build
SRC := $(wildcard *_user.c)
OBJ := $(SRC:%.c=$(BUILD)/%.o)
BPF_SRC := $(wildcard *_kern.c)
BPF_OBJ := $(BPF_SRC:%.c=$(BUILD)/%.o)

LDLIBS += "-lelf -lz"
USER_CFLAGS += -I$(LIB_INSTALL)/include
USER_LDFLAGS += -L$(LIB_INSTALL)/lib -lrt -lcommon -lxdp -lbpf -lelf -lz
BPF_CFLAGS += -I$(LIB_INSTALL)/include

BPFTOOL := $(LIB_INSTALL)/sbin/bpftool
LIBBPF := $(LIB_INSTALL)/lib/libbpf.a
LIBXDP := $(LIB_INSTALL)/lib/libxdp.a
XDP_LOADER := $(LIB_INSTALL)/bin/xdp-loader
COMMON := $(LIB_INSTALL)/lib/libcommon.a

# Detect source file changes
LIBBPF_SOURCES := $(wildcard $(LIB_DIR)/libbpf/src/*.[ch])
BPFTOOL_SOURCES := $(wildcard $(LIB_DIR)/bpftool/src/*.[ch])
LIBXDP_SOURCES := $(wildcard $(LIB_DIR)/xdp-tools/lib/libxdp/libxdp*.[ch]) $(LIB_DIR)/xdp-tools/lib/libxdp/xsk.c
XDP_LOADER_SOURCES := $(wildcard $(LIB_DIR)/xdp-tools/xdp-loader*.[ch])
LIBDPDK_SOURCES := $(wildcard $(LIB_DIR)/dpdk/drivers/*) $(wildcard $(LIB_DIR)/dpdk/lib/*)
COMMON_SOURCES := $(wildcard $(LIB_DIR)/common/*.[ch])
LIB_SOURCES:= $(LIBBPF_SOURCES) $(BPFTOOL_SOURCES) $(LIBXDP_SOURCES) $(XDP_LOADER_SOURCES) $(COMMON_SOURCES)