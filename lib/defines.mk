##
## Common definitions needed to build/install an xdp application
##

LLC ?= llc
CLANG ?= clang
CC := gcc

BUILD := build
SRC := $(wildcard *_user.c)
OBJ := $(SRC:%.c=$(BUILD)/%.o)
BPF_SRC := $(wildcard *_kern.c)
BPF_OBJ := $(BPF_SRC:%.c=$(BUILD)/%.o)

LIB_DIR := lib
LIB_INSTALL := $(LIB_DIR)/install
LIBBPF_DIR := $(LIB_DIR)/libbpf
XDP_TOOLS := $(LIB_DIR)/xdp-tools

BPFTOOL := $(LIB_INSTALL)/sbin/bpftool
OBJECT_LIBBPF := $(LIB_INSTALL)/lib/libbpf.a
OBJECT_LIBXDP := $(LIB_INSTALL)/lib/libxdp.a

CFLAGS := -I$(LIBBPF_DIR)/usr/include -I${XDP_TOOLS}/headers
LDFLAGS := -L$(LIB_INSTALL)/lib -lrt -lelf -lz -lbpf -lxdp
LIBBPF_CFLAGS:=$(if $(CFLAGS),$(CFLAGS),-g -O2 -Werror -Wall) -fPIC

BPF_CFLAGS ?= -I$(LIBBPF_DIR)/usr/include

# install defines
PREFIX?=/usr/local
LIBDIR?=$(PREFIX)/lib
SBINDIR?=$(PREFIX)/sbin
HDRDIR?=$(PREFIX)/include/xdp
DATADIR?=$(PREFIX)/share
MANDIR?=$(DATADIR)/man
BPF_DIR_MNT ?=/sys/fs/bpf
BPF_OBJECT_DIR ?=$(LIBDIR)/bpf
MAX_DISPATCHER_ACTIONS ?=10
BPF_DIR_MNT ?=/sys/fs/bpf
BPF_OBJECT_DIR ?=$(LIBDIR)/bpf

DEFINES := -DBPF_DIR_MNT=\"$(BPF_DIR_MNT)\" -DBPF_OBJECT_PATH=\"$(BPF_OBJECT_DIR)\"

ifeq ($(DEBUG),1)
DEFINES += -DDEBUG
endif

CFLAGS += $(DEFINES)
BPF_CFLAGS += $(DEFINES)
