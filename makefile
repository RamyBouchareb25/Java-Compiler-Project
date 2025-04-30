# Makefile for Java Simplified Compiler

CC = gcc
CFLAGS = -Wall -g
LIBS = -lfl

OBJECTS = lex.yy.o parser.tab.o symbol_table.c semantic_analysis.o intermediate_code.o parser_helper.o global_vars.o diagnostic.o

all: compiler

compiler: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ lex.yy.c parser.tab.c symbol_table.c semantic_analysis.c intermediate_code.c parser_helper.c global_vars.c diagnostic.c $(LIBS)

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y


clean:
	rm -f compiler lex.yy.c parser.tab.c parser.tab.h *.o