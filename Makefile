all:	qoiview

qoiview:
	gcc main.c imageload.c -Wall -lSDL2 -lSDL2_image -lm -o qoiview

clean:
	@rm qoiview
