.DEFAULT_GOAL := reader

CFLAGS := -O2 -std=c11

reader : reader.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -f reader
