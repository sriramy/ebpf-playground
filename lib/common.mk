##
## Common targets needed to build an xdp application
##

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: $(BUILD) $(X) $(BPF_OBJ)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o : %.c
	$(CC) -c -Wall $(CFLAGS) $< -o $@

$(X): $(OBJ)
	$(CC) -o $(X) $(OBJ) $(LDFLAGS)

$(BPF_OBJ): $(BUILD)/%.o: %.c
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

install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(X) $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/lib/bpf
	install -m 644 $(BPF_OBJ) $(DESTDIR)$(PREFIX)/lib/bpf

.PHONY: clean
clean:
	rm -rf $(BUILD)
