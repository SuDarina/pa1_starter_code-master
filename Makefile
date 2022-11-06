
.PHONY: all clean

all: a

clean:
	rm a

a: main.c
	clang -std=c99 -Wall -pedantic *.c

%.c: