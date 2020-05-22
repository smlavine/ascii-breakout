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

/* Rogueutil is a small library which provides simple color and cursor
 * manipulation functions for TUI programs. I am using it to make it easier to
 * move characters around the screen, without having to print a line to send new
 * output. It is based on, and contains several elements from, Tapio Vierros'
 * library "rlutil". Copyright (C) 2020 Sergei Akhmatdinov.
 * The source code can be found here: <https://github.com/sakhmatd/rogueutil>
 */
#include "rogueutil.h"

// Title of the game.
const char *TITLE = "ASCII BREAKOUT";

// Dimensions of the play field.
const int WIDTH = 60;
const int HEIGHT = 36;

// Draws a horizontal bar across the screen.
void
bar(int x, int y, int len, char c)
{
	locate(x, y);
	for (int i = 0; i < len; i++) {
		putchar(c);
	}
}

// Plays a level of the game. Returns the amount of lives remaining at the
// completion of the level, or 0 if the player runs out of lives.
int
play(int level, int *score, int *lives)
{
	// Give the player an extra life every few levels, with the amount of
	// levels in between extra lives increasing as the game goes on. Levels with
	// new lives are in sequence:
	// 1, 2, 3, 4, 6, 8, 12, 16, 20, 29, 33, 37, 41, 45, 54, 58...
	if (level % (1 + level / 5) % 5 == 0) {
		(*lives)++;
	}

	// Initial graphics for the game field are now drawn. This includes a box
	// around the playing field, the score, the paddle, the blocks, etc.
	cls();
	puts(TITLE);
	locate(WIDTH - strlen("SCORE: ") - 8, 1); // Coordinates of "SCORE:" header
	printf("SCORE: %08d", *score);
	// Draws a box around the playfield.
	bar(1, 2, WIDTH + 2, '_');
	for (int y = 3; y < HEIGHT + 3; y++) {
		locate(1, y);
		putchar('{');
		locate(WIDTH + 2, y);
		putchar('}');
	}
	printf("\n{");
	bar(2, HEIGHT + 3, WIDTH, '_');
	putchar('}');

	fflush(stdout);

	// TODO: make proper win/lose test. This is just for testing.
	return 0;
}

int
main(int argc, char *argv[])
{
	int level = 1;
	int score = 0;
	int lives = 5;

	while (play(level, &score, &lives))
		level++;

	puts("\n\n\nGAME OVER");
}

