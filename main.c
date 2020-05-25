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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Rogueutil is a small library which provides simple color and cursor
 * manipulation functions for TUI programs. I am using it to make it easier to
 * move characters around the screen, without having to print a line to send new
 * output. It is based on, and contains several elements from, Tapio Vierros'
 * library "rlutil". Copyright (C) 2020 Sergei Akhmatdinov.
 * The source code can be found here: <https://github.com/sakhmatd/rogueutil>
 */
#include "rogueutil.h"

// Store data about the ball, including location and velocity.
typedef struct {
	// Coordinates of the ball in board.
	int x;
	int y;
	// How many frames the ball moves in each axis. For example, if xVelocity
	// was 1, then the ball would move on the x axis every frame. However, if
	// xVelocity was 3, then the ball would move on the x axis every 3 frames.
	int xVelocity;
	int yVelocity;
	// How many tiles the ball moves on each axis, on a frame where it moves in
	// that axis.
	int xDirection; // negative is left, positive is right.
	int yDirection; // negative is up, positive is down.
} Ball;

// Store data about the paddle, including location and direction.
typedef struct {
	// Coordinates (in board) of the left-most character in the paddle.
	int x;
	int y;
	// Length of the paddle.
	int len;
	// Direction the paddle is moving - negative for left, positive for right.
	int direction;
	// The last direction the paddle was moving before it was frozen.
	int lastDirection;
	// used to control speed of the paddle
	int velocity;
} Paddle;

// Stores data about each tile (character space) on the board, namely, what it
// represents.
typedef enum {
	BALL = 'O',
	PADDLE = MAGENTA,
	RED_BLOCK = RED,
	BLUE_BLOCK = BLUE,
	GREEN_BLOCK = GREEN,
	EMPTY = 0, // A tile not occupied by the ball, paddle, or a block
} Tile;
 

// Dimensions of the play field.
// I used #define instead of const int for these so that I can declare board
// (see below) at file scope.
#define WIDTH 60
#define HEIGHT 36

// Strings for the footer at the bottom of the game board.
const char *TITLE = "ASCII BREAKOUT";
const char *LIVES_FOOTER = "<3:";
const char *LEVEL_FOOTER = "Level:";
const char *SCORE_FOOTER = "Score:";
const int INBETWEEN = 5;
const int FOOTER_XPOS = 4;
const int FOOTER_YPOS = HEIGHT + 2;

// The amount of lives the player starts out with at the beginning of the game.
const int STARTING_LIVES = 5;

// 2D array representing the play field. Randomly generated on each level.
Tile board[WIDTH][HEIGHT];

void bar(int x, int y, int len, char c);
int checkBall(Ball *ball, int *blocksLeft, unsigned int *score,
		unsigned int frame);
void destroyBlock(int x, int y, int *blocksLeft, unsigned int *score);
void drawTile(int x, int y, Tile t);
int generateBoard(const int level, const int maxBlockY,
		Paddle paddle, Ball ball);
void initializeGraphics(int level, unsigned int score,
		int lives);
int max(int a, int b);
int min(int a, int b);
void moveBall(Ball *ball, int x, int y);
void movePaddle(Paddle *paddle);
int play(int level, unsigned int *score, int *lives);
void showMessage(char *fmt, ...);
void updateLevel(int *level);
void updateLives(int *lives);
void updateScore(unsigned int *score);
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

// Checks to see if the ball should move this frame. If it should, then
// moveBall will be called. Also handles collision and bouncing. Returns 0 if
// the ball reaches the bottom of the play field, otherwise returns 1.
int
checkBall(Ball *ball, int *blocksLeft, unsigned int *score, unsigned int frame)
{
	// The new coordinates of the ball, if it moves successfully.
	int nextX = (*ball).x,
		nextY = (*ball).y;


	if (frame % (*ball).xVelocity == 0) {
		nextX += (*ball).xDirection;
	}
	if (frame % (*ball).yVelocity == 0) {
		nextY += (*ball).yDirection;
	}
	
	// Don't do anything if the ball didn't change position.
	if (nextX == (*ball).x && nextY == (*ball).y) {
		return 1;
	}

	// The ball has hit the bottom of the game field.
	if (nextY >= HEIGHT) {
		return 0;
	}

	// if the incoming tile is valid and empty, move there
	if (nextX >= 0 && nextX < WIDTH && nextY >= 0
			&& board[nextX][nextY] == EMPTY) {
		moveBall(ball, nextX, nextY);
	// otherwise, bounce!
	// if stuck in a corner, invert both directions
	} else if (nextY == 0 && (nextX == 0 || nextX == WIDTH - 1)) {
		(*ball).xDirection = -(*ball).xDirection;
		(*ball).yDirection = -(*ball).yDirection;
	// bounce off the side walls
	} else if (nextX <= 0 || nextX >= WIDTH) { 
		(*ball).xDirection = -(*ball).xDirection;
	// bounce off the ceiling
	} else if (nextY <= 0) {
		(*ball).yDirection = -(*ball).yDirection;
	// bounce off paddle
	} else if (board[nextX][nextY] == PADDLE) {
		// If yDirection is not inverted here, then the ball will just roll
		// about on the paddle for a little bit, which is actually kindof fun.
		// Try it out if you're bored.
		(*ball).yDirection = -(*ball).yDirection;
		// randomize bounce a bit
		if (rand() % 2 == 0)
			(*ball).xDirection = -(*ball).xDirection;
		// randomize velocity
		(*ball).xVelocity = (rand() % 8) + 5;
		(*ball).yVelocity = (rand() % 8) + 5;
	// bounce off (and destroy) block
	} else { 
		destroyBlock(nextX, nextY, blocksLeft, score);
		if (rand() % 2 == 0)
			(*ball).xDirection = -(*ball).xDirection;
		if (rand() % 2 == 0)
			(*ball).yDirection = -(*ball).yDirection;
	}
	
	// Indicates that the ball did not hit the bottom of the play field.
	return 1;
}

// Destroys a block at board[x][y], and replaces it with EMPTY. Intended to
// be called when the ball bounces into a block.
void
destroyBlock(int x, int y, int *blocksLeft, unsigned int *score)
{
	// Blocks are generated in groups of two, which means that if one block
	// tile is hit, then one of its neighbors is also going to be destroyed.
	// Because of the way the board is generated, the first tile in a block is
	// always odd. We can use this fact to determine which tile of the block
	// the ball hit: the first or the second. If the x value is odd, then the
	// ball hit the first; if it is even, the the ball hit the second. This
	// is interpreted as an offset to the x value which, when added to the x
	// value, will give us the coordinate of the second tile in the block.
	int offset = (x % 2 == 1) ? 1 : -1;
	board[x][y] = EMPTY;
	updateTile(x, y);
	board[x + offset][y] = EMPTY;
	updateTile(x + offset, y);
	// Remove a block from the total.
	(*blocksLeft)--;
	// Give the player points for destroying a block.
	*score += 10;
	updateScore(score);
}

// Draws at (x, y) [on the terminal window] the proper value depending on the
// tile, including the proper color. Does not reset colors after usage.
void
drawTile(int x, int y, Tile t)
{
	// alternates the character drawn for blocks
	static int alternateBlockChar = 1;
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
		// This helps show the player that blocks are two characters wide.
		setChar(alternateBlockChar ? '(' : ')');
		alternateBlockChar = !alternateBlockChar;
		break;
	case EMPTY:
	default:
		setChar(' ');
		break;
	}
}

// Generates a starting game board. Returns the amount of blocks it generated
// in the level.
int
generateBoard(const int level, const int maxBlockY, Paddle paddle, Ball ball)
{
	// Initializes the board to be empty
	memset(board, EMPTY, WIDTH * HEIGHT * sizeof(Tile));
	// Create the paddle.
	for (int i = 0; i < paddle.len; i++) {
		board[paddle.x + i][paddle.y] = PADDLE;
	}
	// Create the ball.
	board[ball.x][ball.y] = BALL;

	int blocks = 0;
	// Fills in a section of the board with breakable blocks.
	for (int i = 3; i < WIDTH - 3; i += 2) {
		// maxBlockY is the lowest distance the blocks can be generated.
		for (int j = 3; j < maxBlockY; j++) {
			blocks++;
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
			default:
				board[i][j] = GREEN_BLOCK;
				board[i + 1][j] = GREEN_BLOCK;
				break;
			}
		}
	}
	// Formula for this should be (WIDTH - 6)(maxBlockY - 3) / 2, but I'm
	// counting it manually here just to be sure.
	return blocks; 
}

// Draws initial graphics for the game. This includes a box around the playing
// field, the score, the paddle, the blocks, etc.
void
initializeGraphics(int level, unsigned int score, int lives)
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
	// Prints footer information.
	// title
	locate(FOOTER_XPOS, FOOTER_YPOS);
	setColor(CYAN);
	printf("%s", TITLE);
	// lives
	updateLives(&lives);
	// level
	updateLevel(&level);
	// score
	updateScore(&score);
	// Draws the board tiles.
	// i and j refer to y and x so that blocks are drawn in rows, not columns.
	// This makes it easier to produce the two-character wide block effect.
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

// Moves the ball to board[x][y].
void
moveBall(Ball *ball, int x, int y)
{
	board[(*ball).x][(*ball).y] = EMPTY;
	updateTile((*ball).x, (*ball).y);
	(*ball).x = x;
	(*ball).y = y;
	board[x][y] = BALL;
	updateTile(x, y);
}

// Move the paddle according to its direction. 
void
movePaddle(Paddle *paddle)
{
	// The x-coordinate (in board) of which tiles are going to be changed.
	int newPaddleX, newEmptyX; 

	// if paddle is moving left
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
	// if paddle is moving right
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
play(int level, unsigned int *score, int *lives)
{

	// The height of the blocks (how far down on the play field they generate)
	// increases as the levels progress, capping at five-sixths of the height
	// of the board.
	const int maxBlockY = (HEIGHT / 3) + min(level / 2, HEIGHT / 2);

	Paddle paddle;
	// The paddle gets shorter as the game goes on.
	paddle.len = max(20 - (2 * (level / 3)), 10);
	paddle.x = (WIDTH - paddle.len) / 2;
	paddle.y = (11 * HEIGHT) / 12;
	paddle.direction = 0;
	paddle.lastDirection = 0;
	paddle.velocity = 4;

	Ball ball;
	ball.x = WIDTH / 2;
	ball.y = (maxBlockY + paddle.y) / 2;

	// Give the player some extra lives every once in a while, to be nice.
	if (level <= 1) {
		// But not on the first level, since the player starts out with some.
		;
	} else if (level < 10) {
		*lives += 2;
	} else if (level < 20) {
		(*lives)++;
	// Gives out a life every two levels now.
	} else if (level % 2 == 0 && level < 40) {
		(*lives)++;
	// And now every four, until level 60, then the handouts end.
	} else if (level % 4 == 0 && level < 60) {
		(*lives)++;
	}

	// Generates a new board for this level.
	int blocksLeft = generateBoard(level, maxBlockY, paddle, ball);
	
	// A message is printed at the screen at the start of each level/life. It
	// is slightly different if you are not on level 1.
	char *message;
	if (level == 1) {
		message =
			"ASCII Breakout\n"
			"by Sebastian LaVine\n"
			"Press j and k to move the paddle\n"
			"Level: %d\nLives remaining: %d\n"
			"Press any key to continue";
	} else {
		message =
			"Level: %d\nLives remaining: %d\n"
			"Press any key to continue";
	}

	// This is the life loop. In this loop, one life is played out. It can loop
	// many times within one call of play() (a level).
	while (*lives > 0) {
		// Counts how many frames of gameplay have taken place so far.
		unsigned int frame = 0;

		// when the game is paused, the ball freezes and gameplay-related input
		// is frozen.
		int isPaused = 0;

		// The ball resets at the start of each life.
		ball.x = WIDTH / 2;
		ball.y = (maxBlockY + paddle.y) / 2;
		ball.xVelocity = rand() % 10 + 6;
		ball.yVelocity = rand() % 10 + 6;
		ball.xDirection = rand() % 2 == 0 ? 1 : -1;
		ball.yDirection = -1;

		// The paddle recenters itself and resets at the start of each life.
		paddle.x = (WIDTH - paddle.len) / 2;
		paddle.direction = 0;
		paddle.lastDirection = 0;
		// Update paddle tile graphics.
		for (int i = 0; i < WIDTH; i++) {
			board[i][paddle.y] = EMPTY;
		}
		for (int i = 0; i < paddle.len; i++) {
			board[paddle.x + i][paddle.y] = PADDLE;
		}

		// Draws initial graphics for the board.
		initializeGraphics(level, *score, *lives);

		showMessage(message, level, *lives);
		anykey(NULL);
		// Redraw initial graphics to make the message go away.
		initializeGraphics(level, *score, *lives);

		// This is the main game loop. Input is interpreted, tiles move, etc.
		for (;;) {
			// Controls the speed of the game; speed of the ball and the paddle,
			// mainly. Changing this value will also require changing the
			// various velocities of the ball and paddle for gameplay to
			// remain smooth.
			int sleepLength = 5;
			msleep(sleepLength);
			frame++;

			// There is no default case because I want the paddle to continue to
			// move even if there is no input.
			switch (nb_getch()) {
			case 'p':
			case 'P':
				isPaused = !isPaused;
			case 'j': // move the paddle left
			case 'J':
				if (!isPaused) {
					paddle.direction = -1;
					paddle.lastDirection = 0;
				}
				break;
			case 'k': // move the paddle right
			case 'K':
				if (!isPaused) {
					paddle.direction = 1;
					paddle.lastDirection = 0;
				}
				break;
			case 'q': // quits the game.
			case 'Q':
				return 0;
				break;
			case 'r': // redraw the screen. doesn't control the paddle.
			case 'R':
				initializeGraphics(level, *score, *lives);
				break;
			}

			if (!isPaused &&
					paddle.direction != 0 && frame % paddle.velocity == 0) {
				movePaddle(&paddle);
			}

			// I move the cursor out of the way so that inputs that are not
			// caught by nb_getch) are not in the way of the play field.
			//locate(WIDTH + 3, HEIGHT + 3);
			locate(1000, 1000);
			fflush(stdout);

			if (!isPaused && !checkBall(&ball, &blocksLeft, score, frame)) {
				(*lives)--;
				break; // breaks out of input loop
			}
			
			// If there are no blocks remaining, then the player has won and
			// moves on to the next level.
			if (blocksLeft == 0) {
				return 1;
			}

		} // for (;;) (input loop)

		// if the player has ran out of lives, then the game is over.
		if (*lives <= 0) {
			return 0;
		}
	} // life loop

	return 1; // indicates that the player has not ran out of lives
}

// Prints the message in the arguments, formatting it similar to printf,
// however only accepting/recognizing "%d", "%s", and "%c" as valid arguments.
// The lines will be properly centered on the screen.
void
showMessage(char *fmt, ...)
{
	// Copy the fmt string into an array so that strtok() can work with it.
	char fmtCopy[strlen(fmt) + 1];
	strcpy(fmtCopy, fmt);
	
	// This will hold the (formatted) text that will be printed on each line
	// before it is printed.
	char line[WIDTH + 1];

	// Variables to handle the format arguments
	int n;
	char *s;
	va_list ap;
	va_start(ap, fmt);

	char *token = strtok(fmtCopy, "\n");
	for (int lineNumber = 0; token != NULL;
			lineNumber++, token = strtok(NULL, "\n")) {
		memset(line, 0, WIDTH + 1);

		// Count how many printf arguments there are in the current line.
		int argamt = 0;
		for (int i = 0; i < strlen(token) - 1; i++) {
			if (token[i] == '%' && token[i + 1] != '%') {
				argamt++;
			}
		}
		// while the value at *token is not '\0' (the end of the string)
		for (; *token; token++) {
			switch(*token) {
			case '%':
				// if a percentage is found, look for arguments.
				switch (*++token) {
				case 'd':
					n = va_arg(ap, int);
					sprintf(line + strlen(line), "%d", n);
					break;
				case 's':
					s = va_arg(ap, char *);
					sprintf(line + strlen(line), "%s", s);
					break;
				case 'c':
					n = va_arg(ap, int);
					sprintf(line + strlen(line), "%c", n);
					break;
				case '%':
				default:
					line[strlen(line)] = *token;
					break;
				}
				break;
			default:
				line[strlen(line)] = *token;
				break;
			}
		} // while (*token)

		locate(WIDTH / 2 - strlen(line) / 2, HEIGHT / 2 + lineNumber);
		printf("%s", line);

	} // for(...)

	va_end(ap);
}

// Updates the level counter in the footer.
void
updateLevel(int *level)
{
	locate(FOOTER_XPOS + strlen(TITLE) + strlen(LIVES_FOOTER) +
			(INBETWEEN * 2), FOOTER_YPOS);
	setColor(YELLOW);
	printf("%s", LEVEL_FOOTER);
	resetColor();
	printf("%02d\n", *level);
}

// Updates the lives counter in the footer.
void
updateLives(int *lives)
{
	locate(FOOTER_XPOS + strlen(TITLE) + INBETWEEN, FOOTER_YPOS);
	setColor(LIGHTMAGENTA);
	printf("%s", LIVES_FOOTER);
	resetColor();
	printf("%02d\n", *lives);
}

// Updates the score counter in the footer.
void updateScore(unsigned int *score)
{
	locate(FOOTER_XPOS + strlen(TITLE) + strlen(LIVES_FOOTER) +
			strlen(LEVEL_FOOTER) + (INBETWEEN * 3), FOOTER_YPOS);
	setColor(LIGHTCYAN);
	printf("%s", SCORE_FOOTER);
	resetColor();
	printf("%08u\n", *score);
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
	unsigned int score = 0;
	int lives = STARTING_LIVES;

	while (play(level, &score, &lives)) {
		showMessage("Level %d complete!\nPress any key to continue...", level);
		anykey(NULL);
		level++;
	}

	// When the program reaches this point, the player has ran out of lives,
	// and the game is over.
	showMessage("Game over!\nScore: %d\nLevel: %d\nPress any key to quit.",
			score, level);
	anykey(NULL);

	setCursorVisibility(1);
	resetColor();
	locate(1, HEIGHT + 3);
}

