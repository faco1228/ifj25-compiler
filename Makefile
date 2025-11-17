# @file 	Makefile
# @author 	xracekm00
# @brief 	IFJ project - compiler
# @version 	0.6
# @date 	2025-11-17
# 
# @details 	gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
#				
# @copyright Copyright (c) 2025

CC = gcc
CFLAGS = -g -std=c11 -pedantic -Wall -Wextra -O2 -fsanitize=address
LDFLAGS =  -fsanitize=address  # -lm Keby sme nahodou potrebovali matematicku kniznicu

# Pomocka
# target: dependencies
#    activities

all: compiler

compiler: scanner.o error.o lex_funs.o ast.o semantic_analysis.o stack.o parser_expression.o main.o
	$(CC) $(CFLAGS) scanner.o error.o lex_funs.o ast.o semantic_analysis.o stack.o parser_expression.o main.o -o compiler $(LDFLAGS)

scanner.o: scanner.c scanner.h error.h lex_funs.h
	$(CC) $(CFLAGS) -c scanner.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lex_funs.o: lex_funs.c lex_funs.h scanner.h error.h
	$(CC) $(CFLAGS) -c lex_funs.c

ast.o: ast.c ast.h scanner.h error.h
	$(CC) $(CFLAGS) -c ast.c

semantic_analysis.o: semantic_analysis.c semantic_analysis.h scope_stack.h symtable.h error.h ast.h
	$(CC) $(CFLAGS) -c semantic_analysis.c

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c stack.c

parser_expression.o: parser_expression.c parser_expression.h scanner.h stack.h error.h
	$(CC) $(CFLAGS) -c parser_expression.c

main.o: main.c parser.h semantic_analysis.h scope_stack.h symtable.h
	$(CC) $(CFLAGS) -c main.c

# od tadialto nizsie to pred odovzdanim treba zakomentovat pre istotu
run: compiler
	./compiler < test.txt

clean:
	rm -f *.o compiler
