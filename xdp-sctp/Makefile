##
## Make xdp_sctp, to send SCTP packets into an XSK
##

LLC ?= llc
CLANG ?= clang
CC := gcc


BUILD := build
X := $(BUILD)/xdp_sctp_user
SRC := xdp_sctp_user.c
OBJ := $(SRC:%.c=$(BUILD)/%.o)
KVER := linux-6.3
KSRC := $(KERNELDIR)/$(KVER)

XDP_OBJ := $(BUILD)/xdp_sctp_kern.o
BPFD := $(KSRC)/tools/lib/bpf/root
XDP_TOOLS := $(GOPATH)src/github.com/xdp-project/xdp-tools

CFLAGS := -I$(BPFD)/usr/include -I${XDP_TOOLS}/headers -I$(KSRC)/tools/include
LDFLAGS := -L$(BPFD)/usr/lib64 -L$(XDP_TOOLS)/lib/libxdp -lrt -lelf -lz -lbpf -lxdp
BPF_CFLAGS ?= -I$(BPFD)/usr/include

all: $(BUILD) $(X) $(XDP_OBJ)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o : %.c
	$(CC) -c -Wall $(CFLAGS) $< -o $@

$(X): $(OBJ)
	$(CC) -o $(X) $(OBJ) $(LDFLAGS)

$(XDP_OBJ): $(BUILD)/%.o: %.c
	$(CLANG) -S \
		-target bpf \
		-D __BPF_TRACING__ \
		$(BPF_CFLAGS) \
		-Wall \
		-Wno-unused-value \
		-Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Werror \
		-O2 -emit-llvm -c -g -o ${@:.o=.ll} $<
	$(LLC) -march=bpf -filetype=obj -o $@ ${@:.o=.ll}

.PHONY: clean
clean:
	rm -rf $(BUILD)
