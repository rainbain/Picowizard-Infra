
CC       := gcc
TARGET   := build/emu_cli
BUILD    := build

CFLAGS   := -Wall -Wextra -O2 -MMD -MP
CPPFLAGS := -Iinclude
LDFLAGS  :=
LDLIBS   :=

SOURCES  := $(shell find . -type f -name '*.c' -not -path './$(BUILD)/*')
OBJECTS  := $(patsubst ./%.c,$(BUILD)/%.o,$(SOURCES))
DEPS     := $(OBJECTS:.o=.d)

.PHONY: all clean rebuild run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD)

rebuild: clean all
