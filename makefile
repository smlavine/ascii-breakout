ascii-breakout: ./*.c ./*.h
	cc -pedantic -Wall -Werror=implicit-function-declaration ./*.c -o ascii-breakout
