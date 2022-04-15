#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "imageload.h"

#define DEBUG 0

int main(int argc, char** argv);
int render();
int handle_events();
void cleanup();

#define DBG(fmt, ...) do { if (DEBUG) printf(fmt, __VA_ARGS__); } while (0)

// texture
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *surface = NULL;
SDL_Texture *texture = NULL;
SDL_Rect src = {0};

// zoom and move and stuffs
uint8_t fit_to_window = 1; // image appears no bigger than window
double zoom = 1.0; // 100% initially
double move_x = 0.0; // initial x pos (centered)
double move_y = 0.0; // initial y pos (centered)

int
main(int argc, char** argv)
{
	// quit if file not specified
	if(argc < 2)
	{
		fprintf(stderr, "Please provide a filename.\n");
		return 1;
	}

	// quit if file doesn't exist
	if (access(argv[1], F_OK) == -1)
	{
		fprintf(stderr, "Filename doesnt exist.\n");
		return 1;
	}

	// init
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	atexit(cleanup);

	// generate window title
	char* windowtitle = malloc(sizeof(char) * (8 + strlen(argv[1]))); // +8 for length of "SQIV - "
	sprintf(windowtitle, "SQIV - %s", argv[1]);

	// open image file
	SDL_RWops* img;
	img = SDL_RWFromFile(argv[1], "rb");
	if (!img) {
		fprintf(stderr, "Could not open file: %s\n", SDL_GetError());
		return 1;
	}

	// load image
	surface = LoadImageIntoSurface(img);
	SDL_FreeRW(img);
	if (!surface) {
		fprintf(stderr, "Problem while loading the image: (%s).\n", argv[1]);
		return 1;
	}

	// get screen dimensions - 50
	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);
	dm.w -= 50;
	dm.h -= 50;

	// create window
	window = SDL_CreateWindow(
			windowtitle,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			surface->w, surface->h,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	free(windowtitle);

	// create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE);
	texture = SDL_CreateTextureFromSurface(renderer, surface);

	// resize window to image size
	SDL_QueryTexture(texture, NULL, NULL, &src.w , &src.h);
	if (src.w >= dm.w || src.h >= dm.h)
		SDL_MaximizeWindow(window);
	else
		SDL_SetWindowSize(window, src.w, src.h);

	// main event handler
	if (handle_events()) {
		fprintf(stderr, "Event handling exception.\n");
		return 1;
	}

	return 0;
}

int
render()
{
	SDL_Rect dst = {0};
	int screen_h, screen_w;

	SDL_RenderClear(renderer);

	DBG("\nsrc w: %d h: %d x: %d y: %d\n", src.w, src.h, src.x, src.y);

	SDL_GetWindowSize(window, &dst.w, &dst.h);
	screen_h = dst.h;
	screen_w = dst.w;

	DBG("scr w: %d h: %d\n", screen_w, screen_h);

	if (fit_to_window) {
		if (screen_w < src.w || screen_h < src.h) {
			zoom = screen_h;
			zoom /= src.h;
			if (screen_w < src.w * zoom) {
				zoom = screen_w;
				zoom /= src.w;
			}
		} else
			zoom = 1.0;
	}

	/* APPLY ZOOM */
	dst.h = src.h * zoom;
	dst.w = src.w * zoom;

	DBG("zoom: %f\n", zoom);

	/* REVERT IMAGE FILL */
	double ratio = (double) src.w / src.h;
	if (dst.h >= dst.w / ratio)
		dst.h = dst.w / ratio;
	else
		dst.w = ratio * dst.h;

	/* CENTER IN SCREEN */
	dst.x = (screen_w - dst.w) / 2;
	dst.y = (screen_h - dst.h) / 2;

	DBG("dst w: %d h: %d x: %d y: %d\n\n", dst.w, dst.h, dst.x, dst.y);

	/* APPLY MOVE */
	dst.x -= move_x * dst.w;
	dst.y -= move_y * dst.h;

	DBG("Moved by x_perc: %f y_perc: %f\n", move_x, move_y);

	SDL_RenderCopy(renderer, texture, &src, &dst);
	SDL_RenderPresent(renderer);

	return 0;
}

int
handle_events()
{
	SDL_Event event;

	double zoom_multiplier = 0.1; // 10%
	double move_multipler = 0.1; // 10%

	while(SDL_WaitEvent(&event))
	{
		//DBG("Event: 0x%x\n", event.type);
		switch(event.type)
		{
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
					case '+': // zoom in
					case 61: // +
						fit_to_window = 0;
						zoom += zoom_multiplier;
						render();
						break;
					case '-': // zoom out
						fit_to_window = 0;
						zoom -= zoom - zoom_multiplier < 0 ? 0 : zoom_multiplier;
						render();
						break;
					case '0': // reset zoom
						fit_to_window = 0;
						zoom = 1.0;
						render();
						break;
					case '9': // fit to window
						fit_to_window = 1;
						render();
						break;
					case 'k':
					case 1073741906: // up arrow - move up
						move_y -= move_multipler / zoom;
						render();
						break;
					case 'j':
					case 1073741905: // down arrow - move down
						move_y += move_multipler / zoom;
						render();
						break;
					case 'h':
					case 1073741904: // left arrow - move left
						move_x -= move_multipler / zoom;
						render();
						break;
					case 'l':
					case 1073741903: // right arrow - move right
						move_x += move_multipler / zoom;
						render();
						break;
					case 'c': // reset move (center)
						move_x = 0.0;
						move_y = 0.0;
						render();
						break;
					case 'q': // quit
						exit(0);
				}
				break;
			 case SDL_WINDOWEVENT:
				switch (event.window.event) {
					case SDL_WINDOWEVENT_SHOWN:
					case SDL_WINDOWEVENT_EXPOSED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						render(zoom, move_x, move_y);
				}
				break;
			case SDL_QUIT:
				exit(0);
		}
	}

	return 1;
}

void
cleanup()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}
