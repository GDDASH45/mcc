CC       ?= gcc
AR       ?= ar
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude
LDFLAGS  ?=
LDLIBS   ?=

TARGET       := mcc
LIBC_TARGET  := libmcclib.a
SRCDIR       := src
LIBCDIR      := libc/src
OBJDIR       := build
TARGET_ARCH  ?= x86_64

SOURCES      := $(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS      := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

LIBC_SOURCES := $(shell find $(LIBCDIR) -type f -name '*.c')
LIBC_OBJECTS := $(patsubst $(LIBCDIR)/%.c,$(OBJDIR)/libc/%.o,$(LIBC_SOURCES))

LIBC_CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -ffreestanding -fno-builtin -fno-stack-protector -Ilibc/include

ifeq ($(TARGET_ARCH),x86_64)
LIBC_CFLAGS  += -m64
else ifeq ($(TARGET_ARCH),i386)
LIBC_CFLAGS  += -m32
else
$(error Unsupported TARGET_ARCH '$(TARGET_ARCH)'; use x86_64 or i386)
endif

.PHONY: all clean libc

all: $(TARGET) $(LIBC_TARGET)

$(TARGET): $(OBJECTS)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(LIBC_TARGET): $(LIBC_OBJECTS)
	@echo "  AR      $@"
	@$(AR) rcs $@ $^

$(OBJDIR)/libc/%.o: $(LIBCDIR)/%.c
	@echo "  CC      $< ($(TARGET_ARCH) mcclib)"
	@mkdir -p $(dir $@)
	@$(CC) $(LIBC_CFLAGS) -c -o $@ $<

libc: $(LIBC_TARGET)

clean:
	@echo "  CLEAN"
	@rm -rf $(OBJDIR) $(TARGET) $(LIBC_TARGET)
