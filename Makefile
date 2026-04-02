CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := bin/laser

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)
