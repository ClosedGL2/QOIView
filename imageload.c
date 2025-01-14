#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "imageload.h"

SDL_Surface* LoadImageIntoSurface(SDL_RWops* img)
{
	if (is_qoi_image(img))
		return LoadQOI(img);
	else
		return IMG_Load_RW(img, 0);
}

uint8_t is_qoi_image(SDL_RWops* img)
{
	char qoi_magic[5] = {0};

	// read magic from file
	SDL_RWread(img, qoi_magic, 4, 1);
	SDL_RWseek(img, 0, RW_SEEK_SET);

	return strcmp(qoi_magic, "qoif") == 0;
}

SDL_Surface* LoadQOI(SDL_RWops* img)
{
	SDL_Surface* surface;
	Pixel* buf;

	QOIHeader header;
	unsigned int total_pixels;
	unsigned int index = 0;

	Pixel pixel;
	pixel.r = 0;
	pixel.g = 0;
	pixel.b = 0;
	pixel.a = 255;

	Pixel array[64] = {0};
	uint8_t index_position;

	uint8_t byte;
	uint8_t run = 1;
	uint8_t dg;

	// load header
	SDL_RWread(img, &header.magic, 4, 1);

	// all this effort into reading so my little-endian CPU could understand it
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	SDL_RWread(img, &header.width, 4, 1);
	SDL_RWread(img, &header.height, 4, 1);
#else
	SDL_RWread(img, &byte, 1, 1);
	header.width = byte << 24;
	SDL_RWread(img, &byte, 1, 1);
	header.width |= byte << 16;
	SDL_RWread(img, &byte, 1, 1);
	header.width |= byte << 8;
	SDL_RWread(img, &byte, 1, 1);
	header.width |= byte;

	SDL_RWread(img, &byte, 1, 1);
	header.height = byte << 24;
	SDL_RWread(img, &byte, 1, 1);
	header.height |= byte << 16;
	SDL_RWread(img, &byte, 1, 1);
	header.height |= byte << 8;
	SDL_RWread(img, &byte, 1, 1);
	header.height |= byte;
#endif

	SDL_RWread(img, &header.channels, 1, 1);
	SDL_RWread(img, &header.colorspace, 1, 1);

	total_pixels = header.width * header.height;

	// create surface
	surface = SDL_CreateRGBSurface(0,
			header.width, header.height,
			32,
			rmask, gmask, bmask, amask
	);
	if (surface == NULL) {
		fprintf(stderr, "Could not create surface: %s\n", SDL_GetError());
		return NULL;
	}

	buf = surface->pixels;

	// decode image
	while (index < total_pixels) {
		// read byte
		SDL_RWread(img, &byte, 1, 1);

		// decode tag
		switch (byte & 0xc0) {
		case 0xc0: // could be run, rgb, or rgba
			switch (byte) {
			case QOI_OP_RGB: // RGB pixel value
				SDL_RWread(img, &pixel, 3, 1);
				break;
			case QOI_OP_RGBA: // RGBA pixel value
				SDL_RWread(img, &pixel, 4, 1);
				break;
			default: // Run-length encoding
				run = (byte & 0x3f) + 1;
				break;
			}
			break;
		case QOI_OP_INDEX: // index previously seen color
			pixel = array[byte];
			break;
		case QOI_OP_DIFF: // small difference from last pixel
			pixel.r += (((byte & 0x30) >> 4) - 2);
			pixel.g += (((byte & 0x0c) >> 2) - 2);
			pixel.b += ((byte & 0x03) - 2);
			break;
		case QOI_OP_LUMA: // large difference from last pixel
			dg = (byte & 0x3f) - 32;
			pixel.g += dg;
			SDL_RWread(img, &byte, 1, 1);
			pixel.r += ((((byte & 0xf0) >> 4) - 8) + dg);
			pixel.b += (((byte & 0x0f) - 8) + dg);
			break;
		}

		// draw pixel
		for (;run > 0;run--) {
			buf[index] = pixel;
			index++;
		}
		run = 1;

		// add pixel to array
		index_position = ((pixel.r * 3) + (pixel.g * 5) + (pixel.b * 7) + (pixel.a * 11)) % 64;
		array[index_position] = pixel;
	}

	// check for end magic
	for (run = 0; run < 7; run++) {
		SDL_RWread(img, &byte, 1, 1);
		if (byte != 0) return NULL;
	}
	SDL_RWread(img, &byte, 1, 1);
	if (byte != 1) return NULL;

	return surface;
}
