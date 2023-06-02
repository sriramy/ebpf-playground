##
## Common library functions for an XDP application
## BUILD, Q and P must be defined by the calling Makefile
##


LIB_DIR = ..
LIB_INSTALL := $(LIB_DIR)/install
CFLAGS ?= -O2 -g -Werror -Wall -I$(LIB_INSTALL)/include

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
endif

SOURCES := $(wildcard *.c)
OBJ := $(SOURCES:%.c=%.o)
X := libcommon.a

all: $(X)

$(X): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -c -Wall $(CFLAGS) $(EXTRA_CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) $(X)

install: all
	$(Q)install -m 0755 -d $(DESTDIR)/include/common $(P)
	$(Q)install -m 644 *.h -t $(DESTDIR)/include/common $(P)
	$(Q)install -m 0755 -d $(DESTDIR)/lib $(P)
	$(Q)install -m 644 libcommon.a -t $(DESTDIR)/lib $(P)
