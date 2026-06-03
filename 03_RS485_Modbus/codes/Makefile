CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99

SOURCES = rs485_frame_demo.c modbus_crc16.c modbus_rtu_demo.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
