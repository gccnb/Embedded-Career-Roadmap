CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99

TARGETS = linux_serial_demo tcp_client_demo udp_demo

.PHONY: all clean

all: $(TARGETS)

linux_serial_demo: linux_serial_demo.c
	$(CC) $(CFLAGS) $< -o $@

tcp_client_demo: tcp_client_demo.c
	$(CC) $(CFLAGS) $< -o $@

udp_demo: udp_demo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
