SRC := $(wildcard src/*.c)

main: main.c src/minitask.o
	gcc -o main -lpthread -Iinclude main.c src/minitask.o

src/minitask.o: $(SRC)
	gcc -c -o $@ -Wno-deprecated-declarations -Isrc $<

clean:
	rm -f main src/minitask.o

.PHONY: clean