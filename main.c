/**
 * ascii-breakout - a TUI Breakout game
 * Copyright (C) 2020 Sebastian LaVine <seblavine@outlook.com>
 *
 * (MIT License)
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * The software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and noninfringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of conduct, tort or otherwise, arising from,
 * out of or in connection with the software or the use or other deailings in
 * the software.
 */
/* Originally written for submission as the create task for APCSP 2020.
 * The up-to-date source code for this project can be found here:
 * <https://github.com/smlavine/ascii-breakout>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Rogueutil is a small library which provides simple color and cursor
 * manipulation functions for TUI programs. I am using it to make it easier to
 * move characters around the screen, without having to print a line to send new
 * output. It is based on, and contains several elements from, Tapio Vierros'
 * library "rlutil". Copyright (C) 2020 Sergei Akhmatdinov.
 * The source code can be found here: <https://github.com/sakhmatd/rogueutil>
 */
#include "rogueutil.h"

// Type to store data about the location of the paddle on the play field.
typedef struct {
	// Coordinates of the left-most character in the paddle.
	int x;
	int y;
	// Length of the paddle.
	int len;
	// Direction the paddle is moving - negative for left, positive for right.
	int direction;
	// The last direction the paddle was moving before it was frozen.
	int lastDirection;
} Paddle;

// Type to store data about different spaces on the board.
typedef enum {
	BALL = 'o',
	PADDLE = MAGENTA, // WHITE color code will represent the paddle
	RED_BLOCK = RED, // WHITE color code will represent the paddle
	BLUE_BLOCK = BLUE,
	GREEN_BLOCK = GREEN,
	EMPTY = 0, // A tile not occupied by the ball, paddle, or a block
} Tile;
 

// Strings for the header at the top of the game board.
const char *TITLE = "ASCII BREAKOUT";
const char *LIVES_HEADER = "<3:";
const char *LEVEL_HEADER = "Level:";
const char *SCORE_HEADER = "Score:";
const int INBETWEEN = 5;
const int HEADER_XPOS = 4;

// Dimensions of the play field.
// I used #define instead of const int for these so that I can declare board
// (see below) at file scope.
#define WIDTH 60
#define HEIGHT 36

// 2D array representing the play field. Reset after each level.
Tile board[WIDTH][HEIGHT];

void bar(int x, int y, int len, char c);
void drawTile(int x, int y, Tile t);
void generateBoard(const int level, Paddle paddle);
void initializeGraphics(const int level, const int score, const int lives);
int max(int a, int b);
int min(int a, int b);
void movePaddle(Paddle *paddle);
int play(int level, int *score, int *lives);
void updateTile(int x, int y);

// Draws a horizontal bar across the screen.
void
bar(int x, int y, int len, char c)
{
	locate(x, y);
	for (int i = 0; i < len; i++) {
		putchar(c);
	}
}

// Draws at (x, y) the proper value depending on the tile, including the proper
// color. Does not reset colors after usage.
void
drawTile(int x, int y, Tile t)
{
	// alternates the character drawn for bricks
	static int alternateBrickChar = 1;
	resetColor();
	locate(x, y);
	switch (t) {
	case BALL:
		setChar(t);
		break;
	case PADDLE:
		setBackgroundColor(t);
		setChar(' ');
		break;
	case RED_BLOCK:
	case BLUE_BLOCK:
	case GREEN_BLOCK:
		setBackgroundColor(t);
		setColor(BLACK);
		// This helps show the player that bricks are two characters wide.
		setChar(alternateBrickChar ? '(' : ')');
		alternateBrickChar = !alternateBrickChar;
		break;
	case EMPTY:
	default:
		setChar('.');
		break;
	}
}

// Generates a starting game board.
void
generateBoard(const int level, Paddle paddle)
{
	// Initializes the board to be empty
	memset(board, EMPTY, WIDTH * HEIGHT * sizeof(Tile));
	// Create the paddle - It is 5 characters wide.
	for (int i = 0; i < paddle.len; i++) {
		board[paddle.x + i][paddle.y] = PADDLE;
	}
	// Fills in a section of the board with breakable blocks.
	for (int i = 3; i < WIDTH - 3; i += 2) {
		// The amount of blocks increasing through the game, capping at
		// five-sixths the size of the playfield.
		for (int j = 3; j < (HEIGHT / 3) + min(level / 2, HEIGHT / 2); j++) {
			switch (rand() % 3) {
			case 0:
				board[i][j] = RED_BLOCK;
				board[i + 1][j] = RED_BLOCK;
				break;
			case 1:
				board[i][j] = BLUE_BLOCK;
				board[i + 1][j] = BLUE_BLOCK;
				break;
			case 2:
				board[i][j] = GREEN_BLOCK;
				board[i + 1][j] = GREEN_BLOCK;
				break;
			default:
				board[i][j] = EMPTY;
				break;
			}
		}
	}
}

// Draws initial graphics for the game. This includes a box around the playing
// field, the score, the paddle, the blocks, etc.
void
initializeGraphics(const int level, const int score, const int lives)
{
	cls();
	// Draws a box around the game field.
	setColor(GREEN);
	bar(2, 1, WIDTH, '_'); // Top bar
	for (int y = 2; y < HEIGHT + 2; y++) { // Sides of the game field
		locate(1, y);
		putchar('{');
		locate(WIDTH + 2, y);
		putchar('}');
	}
	printf("\n{");
	bar(2, HEIGHT + 2, WIDTH, '_'); // Bottom bar
	putchar('}');
	// Prints header information.
	// title
	locate(HEADER_XPOS, 1);
	setColor(CYAN);
	printf("%s", TITLE);
	// lives
	locate(HEADER_XPOS + strlen(TITLE) + INBETWEEN, 1);
	setColor(LIGHTMAGENTA);
	printf("%s%s%02d\n", LIVES_HEADER, ANSI_LIGHTRED, lives);
	// level
	locate(HEADER_XPOS + strlen(TITLE) + strlen(LIVES_HEADER) +
			(INBETWEEN * 2), 1);
	setColor(YELLOW);
	printf("%s%s%02d\n", LEVEL_HEADER, ANSI_LIGHTRED, level);
	// score
	locate(HEADER_XPOS + strlen(TITLE) + strlen(LIVES_HEADER) +
			strlen(LEVEL_HEADER) + (INBETWEEN * 3), 1);
	setColor(LIGHTCYAN);
	printf("%s%s%08d\n", SCORE_HEADER, ANSI_LIGHTRED, score);
	// Draws the board tiles.
	// i and j refer to y and x so that bricks are drawn in rows, not columns.
	// This makes it easier to produce the two-character wide brick effect.
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			drawTile(j + 2, i + 2, board[j][i]);
		}
	}
	fflush(stdout);
}

// Returns the maximum of two values.
int
max(int a, int b)
{
	return a > b ? a : b;
}

// Returns the minimum of two values.
int
min(int a, int b)
{
	return a < b ? a : b;
}

// Move the paddle in its direction.
void
movePaddle(Paddle *paddle)
{
	// The x-coordinate (in board) of which tiles are going to be changed.
	int newPaddleX, newEmptyX; 

	if ((*paddle).direction < 0 && (*paddle).x + (*paddle).direction >= 0) {
		for (int i = 0; i > (*paddle).direction; i--) {
			newPaddleX = (*paddle).x - 1;
			newEmptyX = (*paddle).x + (*paddle).len - 1;
			board[newPaddleX][(*paddle).y] = PADDLE;
			board[newEmptyX][(*paddle).y] = EMPTY;
			updateTile(newPaddleX, (*paddle).y);
			updateTile(newEmptyX, (*paddle).y);
			(*paddle).x--;
		}
	} else if ((*paddle).direction > 0
			&& (*paddle).x + (*paddle).len + (*paddle).direction <= WIDTH) {
		for (int i = 0; i < (*paddle).direction; i++) {
			newPaddleX = (*paddle).x + (*paddle).len;
			newEmptyX = (*paddle).x;
			board[newPaddleX][(*paddle).y] = PADDLE;
			board[newEmptyX][(*paddle).y] = EMPTY;
			updateTile(newPaddleX, (*paddle).y);
			updateTile(newEmptyX, (*paddle).y);
			(*paddle).x++;
		}
	}
	fflush(stdout);
}

// Plays a level of the game. Returns the amount of lives remaining at the
// completion of the level, or 0 if the player runs out of lives.
int
play(int level, int *score, int *lives)
{
	// Holds information about the location of the paddle, from the left-most
	// character.
	Paddle paddle;
	// The paddle gets shorter as the game goes on.
	paddle.len = max(20 - (2 * (level / 4)), 10);
	paddle.x = (WIDTH - paddle.len) / 2;
	paddle.y = (11 * HEIGHT / 12) + 1;
	paddle.direction = 0;
	paddle.lastDirection = 0;

	// Give the player an extra life every few levels, with the amount of
	// levels in between extra lives increasing as the game goes on. Levels with
	// new lives are in sequence:
	// 1, 2, 3, 4, 6, 8, 12, 16, 20, 29, 33, 37, 41, 45, 54, 58...
	if (level % (1 + level / 5) % 5 == 0) {
		(*lives)++;
	}

	// Generates a new board for this level.
	generateBoard(level, paddle);
	
	// Draws initial graphics for the board.
	initializeGraphics(level, *score, *lives);

	// MAIN GAME LOOP
	for (;;) {
		// Controls how fast the paddle will move.
		msleep(30);
		// There is no default case because I want the paddle to continue to
		// move even if there is no input.
		switch (nb_getch()) {
		case 'f': // freeze/unfreeze the paddle in its place
			if (paddle.lastDirection == 0) { // freeze
				paddle.lastDirection = paddle.direction;
				paddle.direction = 0;
			} else {
				paddle.direction = paddle.lastDirection;
				paddle.lastDirection = 0;
			}
			break;
		case 'j': // move the paddle left
			paddle.direction = -1;
			paddle.lastDirection = 0;
			break;
		case 'k': // move the paddle right
			paddle.direction = 1;
			paddle.lastDirection = 0;
			break;
		case 'q': // quits the game.
			return 0;
			break;
		case 'r': // redraw the screen. doesn't control the paddle.
			initializeGraphics(level, *score, *lives);
			break;
		}

		if (paddle.direction != 0) {
			movePaddle(&paddle);
		}

		// I move the cursor out of the way so that inputs that are not
		// caught by nb_getch) are not in the way of the play field.
		//locate(WIDTH + 3, HEIGHT + 3);
		locate(1000, 1000);
		fflush(stdout);
	}

	locate(1, HEIGHT + 2); // Locate outside of the game board
	// TODO: make proper win/lose test. This is just for testing.
	if (level < 100) {
		return 1;
	} else {
		return 0;
	}
}

// Redraws the tile at board[x][y] in the window. No bounds-checking is done
// here.
void
updateTile(int x, int y)
{
	locate(x + 2, y + 2);
	drawTile(x + 2, y + 2, board[x][y]);
}

int
main(int argc, char *argv[])
{
	srand(time(NULL));
	setCursorVisibility(0);

	int level = 1;
	int score = 0;
	int lives = 4; // At the start of level 1, this will be incremented to 5.

	while (play(level, &score, &lives)) {
		anykey(NULL);
		level++;
	}

	setCursorVisibility(1);
	resetColor();
	locate(1, HEIGHT + 2);
	puts("\nGAME OVER");
}

