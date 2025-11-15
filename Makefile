# @file 	Makefile
# @author 	xracekm00
# @brief 	IFJ project - compiler
# @version 	0.5
# @date 	2025-10-28
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

all: compiler

compiler: scanner.o error.o lex_funs.o
	$(CC) $(CFLAGS) scanner.o error.o lex_funs.o -o compiler $(LDFLAGS)

scanner.o: scanner.c scanner.h error.h lex_funs.h
	$(CC) $(CFLAGS) -c scanner.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lex_funs.o: lex_funs.c lex_funs.h scanner.h error.h
	$(CC) $(CFLAGS) -c lex_funs.c

run: compiler
	./compiler < test.txt

clean:
	rm -f *.o compiler
