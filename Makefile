# @file 	Makefile
# @author 	Martin Racek (xracekm00)
# @brief 	IFJ project - compiler
# @version 	0.7
# @date 	2025-11-17
# 
# @details 	gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
#				
# @copyright Copyright (c) 2025

CC = gcc
CFLAGS = -g -std=c11 -pedantic -Wall -Wextra #-O2 -fsanitize=address
LDFLAGS =  #-fsanitize=address  # -lm Keby sme nahodou potrebovali matematicku kniznicu

# Pomocka
# target: dependencies
#    activities

all: compiler

compiler: scanner.o error.o lex_funs.o ast.o semantic_analysis.o parser.o scope_stack.o symtable.o main.o stack.o parser_expression.o built_in_funs.o code_gen.o
	$(CC) $(CFLAGS) scanner.o error.o lex_funs.o ast.o semantic_analysis.o parser.o scope_stack.o symtable.o main.o stack.o parser_expression.o built_in_funs.o code_gen.o -o compiler $(LDFLAGS)

scanner.o: scanner.c scanner.h error.h lex_funs.h
	$(CC) $(CFLAGS) -c scanner.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lex_funs.o: lex_funs.c lex_funs.h scanner.h error.h
	$(CC) $(CFLAGS) -c lex_funs.c

ast.o: ast.c ast.h scanner.h error.h global_structures.h
	$(CC) $(CFLAGS) -c ast.c

semantic_analysis.o: semantic_analysis.c semantic_analysis.h scope_stack.h symtable.h error.h ast.h global_structures.h
	$(CC) $(CFLAGS) -c semantic_analysis.c

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c stack.c

parser_expression.o: parser_expression.c parser_expression.h scanner.h stack.h error.h global_structures.h
	$(CC) $(CFLAGS) -c parser_expression.c

parser.o: parser.c parser.h scope_stack.h symtable.h semantic_analysis.h global_structures.h
	$(CC) $(CFLAGS) -c parser.c

scope_stack.o: scope_stack.c scope_stack.h symtable.h error.h
	$(CC) $(CFLAGS) -c scope_stack.c

symtable.o: symtable.c symtable.h error.h global_structures.h
	$(CC) $(CFLAGS) -c symtable.c

built_in_funs.o: built_in_funs.c built_in_funs.h
	$(CC) $(CFLAGS) -c built_in_funs.c

code_gen.o: code_gen.c code_gen.h ast.h global_structures.h built_in_funs.h scope_stack.h symtable.h semantic_analysis.h
	$(CC) $(CFLAGS) -c code_gen.c

main.o: compiler_main.c parser.h semantic_analysis.h scope_stack.h symtable.h global_structures.h code_gen.h compiler_main.h ast.h
	$(CC) $(CFLAGS) -c compiler_main.c -o main.o

# od tadialto nizsie to pred odovzdanim treba pre istotu zakomentovat 
run: compiler
	./compiler < test.txt > frantisek.ifjcode || echo "Compiler exited with code $$?"

#	valgrind --leak-check=full --show-leak-kinds=all
 
clean:
	rm -f *.o compiler
