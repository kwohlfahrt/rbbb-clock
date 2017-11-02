.DEFAULT_GOAL := clock.hex

HOST_CFLAGS := -O2 -std=c11
AVR_CFLAGS := -mmcu=atmega328p -DF_CPU=20000000 -DF_OSC=16000000 -Os -std=c11

%.elf: %.c uart.c uart.h
	avr-gcc $(AVR_CFLAGS) $< uart.c -o $@

%.hex: %.elf
	avr-objcopy -O ihex $^ $@

flash-%: %.hex
	avrdude -c arduino -p atmega328p -P /dev/ttyUSB0 -U flash:w:$^

clean:
	rm -f *.hex *.elf
	rm -f reader
