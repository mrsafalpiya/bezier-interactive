#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* PATH_MAX */
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif

/* SDL */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/*
 ===============================================================================
 |                                  Constants                                  |
 ===============================================================================
 */

#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE  "Bezier interactive example"
#define FONT_NAME     "monospace"

#define BUF_MAX 999 /* Max string size and control points */

#define COL_BG 0x000000FF
#define COL_FG 0xFFFFFFFF

#define INFO_MSG \
	"Right click to add/remove control point, Left click to edit a control point.\n'i' to invert color, 't' to toggle info and 'q' to quit."

/*
 ===============================================================================
 |                                   Macros                                    |
 ===============================================================================
 */

#define HEXCOLOR(hex)                                         \
	((hex) >> (3 * 8)) & 0xFF, ((hex) >> (2 * 8)) & 0xFF, \
		((hex) >> (1 * 8)) & 0xFF, ((hex) >> (0 * 8)) & 0xFF

/*
 ===============================================================================
 |                              Global Variables                               |
 ===============================================================================
 */

/* colors */
int col_bg = 0x000000FF;
int col_fg = 0xFFFFFFFF;

/*
 ===============================================================================
 |                                   Structs                                   |
 ===============================================================================
 */

typedef struct {
	float x;
	float y;
} point2d;

/*
 ===============================================================================
 |                            Function Declarations                            |
 ===============================================================================
 */

/* = BEZIER CURVE = */

void
bezier_execute(int division_c, int control_points_n, point2d *control_points,
               int *coefficients, point2d *curve);

void
coefficients_compute(int n, int *coefficients);

void
bezier_point_compute(float parameter, point2d *curve, int control_points_n,
                     point2d *control_points, int *coefficients);

/* = SDL = */

int
SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius);

void
get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text,
                  TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect);

/* = COORDINATE SYSTEM = */

float
point2d_distance(point2d p, float x, float y);

/* = SYSTEM = */

void
get_font_location(const char *font_name, char *loc);

/*
 ===============================================================================
 |                          Function Implementations                           |
 ===============================================================================
 */

/* = BEZIER CURVE = */

void
bezier_execute(int division_c, int control_points_n, point2d *control_points,
               int *coefficients, point2d *curve)
{
	/* coefficients */
	coefficients_compute(control_points_n - 1, coefficients);

	/* bezier points */
	for (int i = 0; i <= division_c; i++)
		bezier_point_compute(i / (float)division_c, &curve[i],
		                     control_points_n, control_points,
		                     coefficients);
}

void
coefficients_compute(int n, int *coefficients)
{
	for (int k = 0; k <= n; k++) {
		/* compute n!/(k!(n-k)!) */
		coefficients[k] = 1;
		for (int i = n; i >= k + 1; i--)
			coefficients[k] *= i;
		for (int i = n - k; i >= 2; i--)
			coefficients[k] /= i;
	}
}

void
bezier_point_compute(float parameter, point2d *curve, int control_points_n,
                     point2d *control_points, int *coefficients)
{
	int n = control_points_n - 1;

	curve->x = 0.0;
	curve->y = 0.0;

	/* Add in influence of each control point */
	for (int k = 0; k < control_points_n; k++) {
		float blend = coefficients[k] * powf(parameter, k) *
		              powf(1 - parameter, n - k);
		curve->x += control_points[k].x * blend;
		curve->y += control_points[k].y * blend;
	}
}

/* = SDL = */

int
SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius)
{
	/* https://gist.github.com/Gumichan01/332c26f6197a432db91cc4327fcabb1c */

	int offsetx, offsety, d;
	int status;

	offsetx = 0;
	offsety = radius;
	d       = radius - 1;
	status  = 0;

	while (offsety >= offsetx) {
		status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
		                             x + offsety, y + offsetx);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
		                             x + offsetx, y + offsety);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
		                             x + offsetx, y - offsety);
		status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
		                             x + offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		} else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		} else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}

void
get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text,
                  TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect)
{
	SDL_Color    textColor = { HEXCOLOR(col_fg) };
	SDL_Surface *surface   = TTF_RenderText_Solid(font, text, textColor);
	if (!surface) {
		fprintf(stderr, "[ERROR] Couldn't render text.\n");
		exit(EXIT_FAILURE);
	}

	*texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!(*texture)) {
		fprintf(stderr, "[ERROR] Couldn't create text texture: %s.\n",
		        SDL_GetError());
		exit(EXIT_FAILURE);
	}

	rect->x = x;
	rect->y = y;
	rect->w = surface->w;
	rect->h = surface->h;

	SDL_FreeSurface(surface);
}

/* = COORDINATE SYSTEM = */

float
point2d_distance(point2d p, float x, float y)
{
	float square_difference_x = (p.x - x) * (p.x - x);
	float square_difference_y = (p.y - y) * (p.y - y);
	float sum                 = square_difference_x + square_difference_y;
	float distance            = sqrt(sum);

	return distance;
}

/* = SYSTEM = */

void
get_font_location(const char *font_name, char *loc)
{
	char cmd[1024] = "/usr/bin/fc-match --format=%{file} ";
	strcat(cmd, font_name);

	FILE *fp = popen(cmd, "r");
	if (fp == NULL) {
		fprintf(stderr,
		        "Failed to get font location (Couldn't run fc-match command).\n");
		exit(EXIT_FAILURE);
	}

	fgets(loc, PATH_MAX, fp);

	pclose(fp);
}

int
main(void)
{
	/* = SDL Initialization = */

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "[ERROR] SDL couldn't be initialized: %s.\n",
		        SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_Window *window =
		SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
	                         SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
	                         WINDOW_HEIGHT, 0);
	if (!window) {
		fprintf(stderr, "[ERROR] Couldn't create window: %s.\n",
		        SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_Renderer *renderer =
		SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		fprintf(stderr, "[ERROR] Couldn't create renderer: %s.\n",
		        SDL_GetError());
		exit(EXIT_FAILURE);
	}

	/* font */
	if (TTF_Init() != 0) {
		fprintf(stderr, "[ERROR] Couldn't initialize font: %s.\n",
		        TTF_GetError());
		exit(EXIT_FAILURE);
	}

	char font_loc[PATH_MAX];
	get_font_location(FONT_NAME, font_loc);
	TTF_Font *font = TTF_OpenFont(font_loc, 24);
	if (!font) {
		fprintf(stderr, "[ERROR] Couldn't open font '%s'.\n",
		        FONT_NAME);
		exit(EXIT_FAILURE);
	}

	/* location text */
	SDL_Texture *texture_mouse_loc_text;
	SDL_Rect     rect_mouse_loc_text;
	get_text_and_rect(renderer, 0, 0, "(0, 0)", font,
	                  &texture_mouse_loc_text, &rect_mouse_loc_text);

	/* = BEZIER CURVE = */

	int     control_points_n = 0;
	point2d control_points[BUF_MAX];

	int     division_c = 1000;
	int     coefficients[BUF_MAX];
	point2d curve[division_c + 1];

	/* = DRAWING = */

	/* mouse location */
	int mouse_x = 0;
	int mouse_y = 0;

	/* main loop */
	int      running                = 1;
	int      selected               = 0;
	int      show_info              = 1;
	point2d *control_point_selected = NULL;
	char     mouse_loc_str[BUF_MAX] = "(0, 0)";

	printf(INFO_MSG "\n");
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_MOUSEMOTION: {
				mouse_x = event.motion.x;
				mouse_y = event.motion.y;

				/* location on screen */
				sprintf(mouse_loc_str, "(%d, %d)", mouse_x,
				        mouse_y);
			} break;
			case SDL_MOUSEBUTTONDOWN: {
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					if (selected) {
						printf("Changed control point #%lu from (%d, %d) to (%d, %d)\n",
						       control_point_selected -
						               control_points +
						               1,
						       (int)control_point_selected
						               ->x,
						       (int)control_point_selected
						               ->y,
						       mouse_x, mouse_y);
						control_point_selected->x =
							mouse_x;
						control_point_selected->y =
							mouse_y;

						bezier_execute(division_c,
						               control_points_n,
						               control_points,
						               coefficients,
						               curve);

						selected = 0;
						break;
					}
					for (int i = 0; i < control_points_n;
					     i++) {
						if (point2d_distance(
							    control_points[i],
							    mouse_x,
							    mouse_y) < 10) {
							selected = 1;
							control_point_selected =
								control_points +
								i;
							break;
						}
					}
					break;
				case SDL_BUTTON_RIGHT:
					selected = 0;
					for (int i = 0; i < control_points_n;
					     i++) {
						if (point2d_distance(
							    control_points[i],
							    mouse_x,
							    mouse_y) < 10) {
							/* remove the control point */
							printf("Removed control point #%d (%d, %d)\n",
							       i + 1,
							       (int)control_points
							               [i]
							                       .x,
							       (int)control_points
							               [i]
							                       .y);
							memmove(control_points +
							                i,
							        control_points +
							                i + 1,
							        (control_points_n -
							         i +
							         1) * sizeof(point2d));
							control_points_n--;
							goto update;
						}
					}
					/* add the control point */
					control_points[control_points_n].x =
						mouse_x;
					control_points[control_points_n].y =
						mouse_y;
					control_points_n++;
					printf("Added control point #%d (%d, %d)\n",
					       control_points_n, mouse_x,
					       mouse_y);

				update:
					bezier_execute(division_c,
					               control_points_n,
					               control_points,
					               coefficients, curve);
				}
			} break;
			case SDL_KEYDOWN: {
				switch (event.key.keysym.sym) {
				case SDLK_i: {
					int col_temp;
					col_temp = col_fg;
					col_fg   = col_bg;
					col_bg   = col_temp;
				} break;
				case SDLK_t: {
					show_info = !show_info;
				} break;
				case SDLK_q: {
					running = 0;
				} break;
				}
			} break;
			case SDL_QUIT: {
				running = 0;
			} break;
			}
		}

		/* clear the render */
		SDL_SetRenderDrawColor(renderer, HEXCOLOR(col_bg));
		SDL_RenderClear(renderer);

		/* draw control points */
		if (show_info) {
			for (int i = 0; i < control_points_n; i++) {
				if (selected && (control_points + i ==
				                 control_point_selected))
					SDL_SetRenderDrawColor(renderer, 0, 255,
					                       0, 255);
				else
					SDL_SetRenderDrawColor(renderer, 255, 0,
					                       0, 255);
				SDL_RenderFillCircle(renderer,
				                     control_points[i].x,
				                     control_points[i].y, 3);
			}
		}

		/* draw bezier curve */
		SDL_SetRenderDrawColor(renderer, HEXCOLOR(col_fg));
		for (int i = 0; i < division_c + 1; ++i)
			SDL_RenderDrawPoint(renderer, curve[i].x, curve[i].y);

		/* draw text */
		if (show_info) {
			SDL_DestroyTexture(texture_mouse_loc_text);
			get_text_and_rect(renderer, 0, 0, mouse_loc_str, font,
			                  &texture_mouse_loc_text,
			                  &rect_mouse_loc_text);
			SDL_RenderCopy(renderer, texture_mouse_loc_text, NULL,
			               &rect_mouse_loc_text);
		}
		SDL_RenderPresent(renderer);
	}

	/* = EXIT = */
	TTF_CloseFont(font);
	SDL_DestroyTexture(texture_mouse_loc_text);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
