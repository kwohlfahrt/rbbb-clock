.DEFAULT_GOAL := clock.hex

CFLAGS := -mmcu=atmega328p -DF_CPU=20000000 -DF_OSC=16000000 -Os -std=c11

%.elf: %.c uart.c uart.h
	avr-gcc $(CFLAGS) $< uart.c -o $@

%.hex: %.elf
	avr-objcopy -O ihex $^ $@
