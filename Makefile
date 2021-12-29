ascii-breakout: ./*.c ./*.h
	cc -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration ./*.c -o ascii-breakout
