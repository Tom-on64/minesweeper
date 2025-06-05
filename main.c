#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifndef VERSION
#warning "VERSION macro was not defined!"
#define VERSION "[NO VERSION]"
#endif

#include "config.h"

// Color helper macro
#define COLOR(_c) SDL_SetRenderDrawColor(\
		renderer,\
		((_c) >> 16) & 0xFF,\
		((_c) >> 8)  & 0xFF,\
		((_c) >> 0)  & 0xFF,\
		0xFF\
		)

enum {
	S_FIRST_MOVE,
	S_IN_GAME,
	S_WIN,
	S_LOSE,
};

typedef struct {
	int mine;
	int revealed;
	int flagged;
} cell_t;

struct {
	int w;
	int h;
	int c;
	int u;
	cell_t* cells;
} grid;

int quit = 0;
int state = S_FIRST_MOVE;
char* exename = NULL;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

cell_t* cellat(int x, int y);
void init_grid(int w, int h, int c);
int count_mines(int x, int y);
void draw_text(char* txt, uint32_t col, SDL_Rect* rect);
void draw_grid(void);
void handle_events(int* quit);
void reveal_tile(int x, int y);
void flag_tile(int x, int y);
void cleanup(void);
void win(void);
void lose(void);

int main(int argc, char** argv) {
	exename = argv[0];

	int width = DEF_WIDTH;
	int height = DEF_HEIGHT;
	int mines = DEF_MINES;
	
	// Argument parsing :(
	for (int i = 1; i < argc; i++) {
		char* arg = argv[i];

		if (arg[0] != '-') continue;

		switch (arg[1]) {
		case 'd':
			width = atoi(argv[++i]);
			height = atoi(argv[++i]);
			break;
		case 'h':
			printf("Usage: %s [-hv] [-m <count>] [-d <width> <height>]\n", exename);
			return 0;
		case 'm': mines = atoi(argv[++i]); break;
		case 'v': 
			printf("%s: V%s\n", exename, VERSION);
			return 0;
		default:
			fprintf(stderr, "%s: Unknown option -%c.\n", exename, arg[1]);
			return 1;
		}
	}
	
	if (SDL_Init(SDL_INIT_VIDEO) != 0) cleanup();
	if (TTF_Init() != 0) cleanup();

	window = SDL_CreateWindow(WIN_TITLE, 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width * TILE_SIZE, height * TILE_SIZE, SDL_WINDOW_SHOWN
	);
	if (!window) cleanup();

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) cleanup();

	font = TTF_OpenFont(FONT, FONT_SIZE);
	if (!font) cleanup();

	init_grid(width, height, mines);

	while (!quit) {
		if (grid.u < grid.c) win();

		handle_events(&quit);

		draw_grid();

		SDL_Rect rect = {
			24, 
			24,
			(FONT_SIZE * ((state == S_WIN) ? 10 : 13)) * 2,
			(FONT_SIZE) * 4,
		};

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0x33);
		if (state == S_WIN) {
			SDL_RenderFillRect(renderer, &rect);
			draw_text(" You Won! ", COL_WIN, &rect);
		} else if (state == S_LOSE) {

			SDL_RenderFillRect(renderer, &rect);
			draw_text(" You Lost :( ", COL_LOSE, &rect);
		}

		SDL_RenderPresent(renderer);
		SDL_Delay(5);
	}

	cleanup();
}

cell_t* cellat(int x, int y) {
	if (x < 0 || x >= grid.w || y < 0 || y >= grid.h) return NULL;
	int pos = y * grid.w + x;

	return &grid.cells[pos];
}

void init_grid(int w, int h, int c) {
	grid.cells = malloc(w * h * sizeof(*grid.cells));
	memset(grid.cells, 0, w * h * sizeof(*grid.cells));
	grid.w = w;
	grid.h = h;
	grid.c = c;
	grid.u = grid.w * grid.h;

	// Generate minefield
	srand(time(NULL));
	for (int i = 0; i < grid.c; i++) {
		int x = rand() % grid.w;
		int y = rand() % grid.h;

		cellat(x, y)->mine = 1;
	}
}

int count_mines(int x, int y) {
	int n = 0;

	#define _check_mine(_x, _y) if (cellat((_x), (_y)) && cellat((_x), (_y))->mine) n++;

	_check_mine(x - 1, y - 1);
	_check_mine(x,     y - 1);
	_check_mine(x + 1, y - 1);
	_check_mine(x - 1, y    );
	_check_mine(x,     y    );
	_check_mine(x + 1, y    );
	_check_mine(x - 1, y + 1);
	_check_mine(x,     y + 1);
	_check_mine(x + 1, y + 1);

	return n;
}

void draw_text(char* txt, uint32_t col, SDL_Rect* rect) {
	SDL_Color text_col = { 
		(col >> 16) & 0xff,
		(col >> 8)  & 0xff,
		(col)       & 0xff,
		0xff,
	};

	SDL_Surface* surface = TTF_RenderText_Solid(font, txt, text_col);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_RenderCopy(renderer, texture, NULL, rect);

	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}

void draw_grid(void) {
	for (int i = 0; i < grid.w * grid.h; i++) {
		int x = i % grid.w;
		int y = i / grid.w;

		cell_t* cell = cellat(x, y);
		SDL_Rect rect = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };

		int mine_count = count_mines(x, y);
		
		if (!cell->revealed && cell->flagged) COLOR(COL_FLAG);
		else if (!cell->revealed) COLOR(COL_TILE);
		else if (cell->mine) COLOR(COL_MINE);
		else switch(mine_count) {
			case 0: COLOR(COL_E0); break;
			case 1: COLOR(COL_E1); break;
			case 2: COLOR(COL_E2); break;
			case 3: COLOR(COL_E3); break;
			case 4: COLOR(COL_E4); break;
			case 5: COLOR(COL_E5); break;
			case 6: COLOR(COL_E6); break;
			case 7: COLOR(COL_E7); break;
			case 8: COLOR(COL_E8); break;
			case 9: COLOR(COL_E9); break;
		}

		char msg[2] = { '0' + mine_count, '\0' };


		SDL_RenderFillRect(renderer, &rect);
		COLOR(COL_BORDER);
		SDL_RenderDrawRect(renderer, &rect);
		if (cell->revealed && !cell->mine && mine_count > 0) {
			draw_text(msg, COL_FONT, &rect);
		}

	}
}

void handle_events(int* quit) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			*quit = 1;
			continue;
		} else if (e.type != SDL_MOUSEBUTTONDOWN) continue;

		int x = e.button.x / TILE_SIZE;
		int y = e.button.y / TILE_SIZE;

		if (e.button.button == SDL_BUTTON_LEFT) reveal_tile(x, y);
		else if (e.button.button == SDL_BUTTON_RIGHT) flag_tile(x, y);
	}
}

void reveal_tile(int x, int y) {
	cell_t* cell = cellat(x, y);
	if (cell == NULL) return;

	if (cell->revealed || cell->flagged) return;

	if (cell->mine && state == S_FIRST_MOVE) {
		init_grid(grid.w, grid.h, grid.c);
		reveal_tile(x, y);
		return;
	}

	state = S_IN_GAME;
	cell->revealed = 1;
	grid.u--;
	if (cell->mine) {
		lose();
	}

	if (count_mines(x, y) == 0) {
		reveal_tile(x + 1, y + 1);
		reveal_tile(x + 0, y + 1);
		reveal_tile(x - 1, y + 1);
		reveal_tile(x + 1, y + 0);
		reveal_tile(x - 1, y + 0);
		reveal_tile(x + 1, y - 1);
		reveal_tile(x + 0, y - 1);
		reveal_tile(x - 1, y - 1);
	}
}

void flag_tile(int x, int y) {
	cell_t* cell = cellat(x, y);
	cell->flagged = !cell->flagged;
}

void win(void) {
	for (int i = 0; i < grid.w * grid.h; i++) grid.cells[i].revealed = 1;
	state = S_WIN;
}

void lose(void) {
	for (int i = 0; i < grid.w * grid.h; i++) grid.cells[i].revealed = 1;
	state = S_LOSE;
}

__attribute__ ((noreturn))
void cleanup(void) {
	if (grid.cells) free(grid.cells);
	if (font) TTF_CloseFont(font);
	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);

	SDL_Quit();
	exit(0);
}

