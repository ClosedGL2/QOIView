#ifndef sqiv_imageload
#define sqiv_imageload

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdint.h>

SDL_Surface* LoadImageIntoSurface(SDL_RWops* img);

SDL_Surface* LoadQOI(SDL_RWops* img);

uint8_t is_qoi_image(SDL_RWops* img);

typedef struct {
	char magic[4];
	uint32_t width;
	uint32_t height;
	uint8_t channels;
	uint8_t colorspace;
} QOIHeader;

typedef struct {
	uint8_t r, g, b, a;
} Pixel;

#define QOI_OP_RGB		0xfe
#define QOI_OP_RGBA		0xff
#define QOI_OP_INDEX	0x00
#define QOI_OP_DIFF		0x40
#define QOI_OP_LUMA		0x80

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff000000
#define gmask 0x00ff0000
#define bmask 0x0000ff00
#define amask 0x000000ff
#else
#define rmask 0x000000ff
#define gmask 0x0000ff00
#define bmask 0x00ff0000
#define amask 0xff000000
#endif

#endif // sqiv_imageload
