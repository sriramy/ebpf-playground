##
## Common definitions needed to build an xdp application
##

LLC ?= llc
CLANG ?= clang
CC := gcc

BUILD := build
KVER := linux-6.3
KSRC := $(KERNELDIR)/$(KVER)
SRC := $(wildcard *_user.c)
OBJ := $(SRC:%.c=$(BUILD)/%.o)
BPF_SRC := $(wildcard *_kern.c)
BPF_OBJ := $(BPF_SRC:%.c=$(BUILD)/%.o)

BPFD := $(KSRC)/tools/lib/bpf/root
XDP_TOOLS := $(GOPATH)src/github.com/xdp-project/xdp-tools

CFLAGS := -I$(BPFD)/usr/include -I${XDP_TOOLS}/headers -I$(KSRC)/tools/include
LDFLAGS := -L$(BPFD)/usr/lib64 -L$(XDP_TOOLS)/lib/libxdp -lrt -lelf -lz -lbpf -lxdp
BPF_CFLAGS ?= -I$(BPFD)/usr/include
