# @file 	Makefile
# @author 	xracekm00
# @brief 	IFJ project - compiler
# @version 	0.4
# @date 	2025-10-26
# 
# @details 	gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
#				
# @copyright Copyright (c) 2025

CC = gcc
CFLAGS = -g -std=c11 -pedantic -Wall -Wextra -O2 -fsanitize=address
LDFLAGS = -lm -fsanitize=address  # Pridanie matematickej knižnice a overenie práce s pamäťou

# Pomocka
# target: dependencies
#    activities

all: test_scanner #cpmpiler

#compiler: scanner.o error.o lex_funs.o
#	$(CC) $(CFLAGS) scanner.o error.o lex_funs.o -o compiler $(LDFLAGS)

test_scanner: test.o scanner.o lex_funs.o error.o
	$(CC) $(CFLAGS) test.o scanner.o lex_funs.o error.o -o test_scanner $(LDFLAGS)
	
test.o: test.c scanner.h error.h
	$(CC) $(CFLAGS) -c test.c

scanner.o: scanner.c scanner.h error.h lex_funs.h
	$(CC) $(CFLAGS) -c scanner.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lex_funs.o: lex_funs.c lex_funs.h scanner.h error.h
	$(CC) $(CFLAGS) -c lex_funs.c

run: test_scanner
	./test_scanner < test1.txt

clean:
	rm -f *.o compiler test_scanner
