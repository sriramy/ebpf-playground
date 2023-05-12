##
## Common targets needed to build an xdp application
##

all: $(BUILD) $(BUILD)/$(X) $(BPF_OBJ)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o : %.c
	$(CC) -c -Wall $(USER_CFLAGS) $< -o $@

$(BUILD)/$(X): $(OBJ)
	$(CC) -o $(BUILD)/$(X) $(OBJ) $(USER_LDFLAGS)

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
	install -m 755 $(BUILD)/$(X) $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/lib/bpf
	install -m 644 $(BPF_OBJ) $(DESTDIR)$(PREFIX)/lib/bpf

.PHONY: clean
clean:
	rm -rf $(BUILD)
