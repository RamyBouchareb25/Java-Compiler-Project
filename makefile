# Makefile for Java Simplified Compiler

CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -g

all: compiler

compiler: lexer.l parser.y symbol_table.c semantic_analysis.c intermediate_code.c
	$(BISON) -d parser.y
	$(FLEX) lexer.l
	$(CC) $(CFLAGS) -o compiler lex.yy.c parser.tab.c symbol_table.c semantic_analysis.c intermediate_code.c -lfl

clean:
	rm -f compiler lex.yy.c parser.tab.c parser.tab.h *.o