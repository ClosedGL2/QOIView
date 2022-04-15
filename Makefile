all: sqiv

sqiv: main.o imageload.o
	gcc *.o -Wall -lSDL2 -lSDL2_image -lm -o sqiv

main.o: main.c
	gcc -c main.c -Wall

imageload.o: imageload.c
	gcc -c imageload.c -Wall

clean:
	rm *.o
