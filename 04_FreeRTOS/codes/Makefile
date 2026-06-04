CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic

TARGETS := freertos_rs485_arch_demo

.PHONY: all clean

all: $(TARGETS)

freertos_rs485_arch_demo: freertos_rs485_arch_demo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o *.exe
