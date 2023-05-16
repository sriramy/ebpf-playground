##
## Common library functions for an XDP application
## BUILD, Q and P must be defined by the calling Makefile
##


LIB_DIR = ..
LIB_INSTALL := $(LIB_DIR)/install
CFLAGS ?= -O2 -g -Werror -Wall -I$(LIB_INSTALL)/include

COMMON_INCLUDE := $(wildcard *.h)
COMMON_SOURCES := $(wildcard *.c)
COMMON_OBJ := $(COMMON_SOURCES:%.c=%.o)
COMMON_AR := libcommon.a

all: $(COMMON_AR)

$(COMMON_AR): $(COMMON_OBJ)
	$(AR) rcs $@ $^

$(COMMON_OBJ): $(COMMON_SOURCES)
	$(CC) -c -Wall $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(COMMON_OBJ) $(COMMON_AR)
