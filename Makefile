CC ?= arm-kindle5-linux-gnueabi-gcc
CFLAGS ?= -O2 -Wall -Wextra -std=gnu11 -Iinclude -D_GNU_SOURCE
LDFLAGS ?= -lm

TARGET = aurabooks_bin
SRC = src/main.c \
      src/core/aura_fb.c \
      src/core/aura_power.c \
      src/input/aura_touch.c \
      src/gfx/aura_font.c \
      src/gfx/aura_image.c \
      src/db/aura_db.c \
      src/ui/aura_ui.c \
      src/reader/aura_reader.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
