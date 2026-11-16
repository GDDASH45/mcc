CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude
LDFLAGS  ?=
LDLIBS   ?=

TARGET   := mcc
SRCDIR   := src
OBJDIR   := build

SOURCES  := $(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	@echo "  CLEAN"
	@rm -rf $(OBJDIR) $(TARGET)
