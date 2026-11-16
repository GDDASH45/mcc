CC       ?= gcc
AR       ?= ar
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude
LDFLAGS  ?=
LDLIBS   ?=

TARGETS      := mcc mcc-as mcc-ld mcc-cpp
LIBC_TARGET  := libmcclib.a
SRCDIR       := src
LIBCDIR      := libc/src
OBJDIR       := build
TARGET_ARCH  ?= x86_64

MCC_SOURCES  := $(shell find $(SRCDIR) -maxdepth 2 -type f -name '*.c' ! -path '$(SRCDIR)/as/*' ! -path '$(SRCDIR)/ld/*' ! -path '$(SRCDIR)/cpp/*')
AS_SOURCES   := $(shell find $(SRCDIR)/as -type f -name '*.c')
LD_SOURCES   := $(shell find $(SRCDIR)/ld -type f -name '*.c')
CPP_SOURCES  := $(shell find $(SRCDIR)/cpp -type f -name '*.c')
LIBC_SOURCES := $(shell find $(LIBCDIR) -type f -name '*.c')

MCC_OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MCC_SOURCES))
AS_OBJECTS   := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(AS_SOURCES))
LD_OBJECTS   := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LD_SOURCES))
CPP_OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CPP_SOURCES))
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

all: $(TARGETS) $(LIBC_TARGET)

mcc: $(MCC_OBJECTS)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mcc-as: $(AS_OBJECTS)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mcc-ld: $(LD_OBJECTS)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mcc-cpp: $(CPP_OBJECTS)
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
	@rm -rf $(OBJDIR) $(TARGETS) $(LIBC_TARGET)
