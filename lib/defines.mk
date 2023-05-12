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
ifeq ($(VERBOSE),0)
P:= >/dev/null
MAKEFLAGS += --no-print-directory
endif

LLC ?= llc
CLANG ?= clang
CC := gcc

PREFIX?=/usr/local
LIBDIR?=$(PREFIX)/lib
SBINDIR?=$(PREFIX)/sbin
HDRDIR?=$(PREFIX)/include/xdp
DATADIR?=$(PREFIX)/share
MANDIR?=$(DATADIR)/man

CFLAGS ?= -O2 -g
BPF_CFLAGS ?= -Wno-visibility

ifeq ($(DEBUG),1)
DEFINES += -DDEBUG
Q :=
endif

CFLAGS += $(DEFINES)
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

USER_CFLAGS := -I$(LIB_INSTALL)/include -I${XDP_TOOLS}/headers
USER_LDFLAGS := -L$(LIB_INSTALL)/lib -lrt -lxdp -lbpf -lelf -lz
BPF_CFLAGS += -I$(LIB_INSTALL)/include

BPFTOOL := $(LIB_INSTALL)/sbin/bpftool
LIBBPF := $(LIB_INSTALL)/lib/libbpf.a
LIBXDP := $(LIB_INSTALL)/lib/libxdp.a
XDP_LOADER := $(LIB_INSTALL)/bin/xdp-loader
LIBBPF_CFLAGS := $(if $(CFLAGS),$(CFLAGS),-g -O2 -Werror -Wall) -fPIC

CFLAGS += "${USER_CFLAGS}"
LDLIBS += "-lelf -lz"