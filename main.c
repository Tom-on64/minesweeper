#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define WIN_TITLE	"Minesweeper"
#define DEF_WIDTH	32
#define DEF_HEIGHT	16
#define DEF_MINES	64
#define TILE_SIZE	32
#define FONT		"/usr/share/fonts/TTF/DejaVuSansMono.ttf"
#define FONT_SIZE	16
#define COL_FONT	0xCCCCCC
#define COL_BORDER	0x000000
#define COL_TILE	0xAAAAAA
#define COL_MINE	0x0C0C0C
#define COL_FLAG	0xFFFF00
#define COL_E0		0x777777
#define COL_E1		0x0000FF
#define COL_E2		0x008000
#define COL_E3		0xFF0000
#define COL_E4		0x000060
#define COL_E5		0x600000
#define COL_E6		0x008080
#define COL_E7		0x800080
#define COL_E8		0x777777
#define COL_E9		0xFFFFFF
#define COLOR(_c)	SDL_SetRenderDrawColor(\
		renderer,\
		((_c) >> 16) & 0xFF,\
		((_c) >> 8)  & 0xFF,\
		((_c) >> 0)  & 0xFF,\
		0xFF\
		)

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
int first_move = 1;
char* exename = NULL;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

cell_t* cellat(int x, int y);
void init_grid(int w, int h, int c);
int count_mines(int x, int y);
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
			printf("Usage: %s [-m <count>] [-d <width> <height>]\n", exename);
			return 0;
		case 'm': mines = atoi(argv[++i]); break;
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
		if (grid.u <= grid.c) win();

		handle_events(&quit);
		draw_grid();
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

#define _check_mine(_x, _y) if (cellat((_x), (_y)) && cellat((_x), (_y))->mine) n++;
int count_mines(int x, int y) {
	int n = 0;

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

		SDL_Color text_col = { 
			(COL_FONT >> 16)& 0xff,
			(COL_FONT >> 8)	& 0xff,
			(COL_FONT)	& 0xff,
			0xff,
		};
		SDL_Surface* surface = TTF_RenderText_Solid(font, msg, text_col);
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

		SDL_RenderFillRect(renderer, &rect);
		COLOR(COL_BORDER);
		SDL_RenderDrawRect(renderer, &rect);
		if (cell->revealed && !cell->mine && mine_count > 0) {
			SDL_RenderCopy(renderer, texture, NULL, &rect);
		}

		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surface);
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

	if (cell->revealed || cell->flagged) return;

	grid.u--;
	if (cell->mine && first_move) {
		init_grid(grid.w, grid.h, grid.c);
		reveal_tile(x, y);
		return;
	}

	first_move = 0;
	cell->revealed = 1;
	if (cell->mine) {
		lose();
	}
}

void flag_tile(int x, int y) {
	cell_t* cell = cellat(x, y);
	cell->flagged = !cell->flagged;
	if (cell->mine && cell->flagged) grid.u--;
	else if (cell->mine) grid.u++;
}

void win(void) {
	printf("You won!\n");
	for (int i = 0; i < grid.w * grid.h; i++) grid.cells[i].revealed = 1;
}

void lose(void) {
	printf("Game over!\n");
	SDL_Delay(1000);
	for (int i = 0; i < grid.w * grid.h; i++) grid.cells[i].revealed = 1;
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

