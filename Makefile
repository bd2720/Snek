all: snek

snek: snek.o
	gcc snek.o -o snek

snek.o: snek.c
	gcc -g -c snek.c

clean:
	rm -rf *.o snek
