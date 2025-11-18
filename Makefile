# @file 	Makefile
# @brief 	IFJ project - scanner test build
# @version 	0.1

CC = gcc
CFLAGS = -g -std=c11 -pedantic -Wall -Wextra -O2 #-fsanitize=address
LDFLAGS = #-fsanitize=address

all: scanner_test

scanner_test: scanner.o lex_funs.o error.o scanner_test.o
	$(CC) $(CFLAGS) scanner.o lex_funs.o error.o scanner_test.o -o scanner_test $(LDFLAGS)

scanner.o: scanner.c scanner.h error.h lex_funs.h
	$(CC) $(CFLAGS) -c scanner.c

lex_funs.o: lex_funs.c lex_funs.h scanner.h error.h
	$(CC) $(CFLAGS) -c lex_funs.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

scanner_test.o: scanner_test.c scanner.h
	$(CC) $(CFLAGS) -c scanner_test.c

run: scanner_test
	./scanner_test < test.txt || echo "Compiler exited with code $$?"

clean:
	rm -f *.o scanner_test
