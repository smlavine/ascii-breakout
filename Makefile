ascii-breakout: ./*.c ./*.h
	cc -std=c99 -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration ./*.c -o ascii-breakout
