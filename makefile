ascii-breakout: ./*.c ./*.h
	cc -g -pedantic -Wall -Werror=implicit-function-declaration ./*.c -o ascii-breakout
