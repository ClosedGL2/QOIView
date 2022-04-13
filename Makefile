all:	sqiv

sqiv:
	gcc main.c imageload.c -Wall -lSDL2 -lSDL2_image -lm -o sqiv

clean:
	@rm sqiv
