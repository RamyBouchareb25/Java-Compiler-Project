# Makefile for Java Simplified Compiler

CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -g
LDFLAGS = -lfl

SOURCES = main.c lex.yy.c parser.tab.c ast.c symbol_table.c semantic_analysis.c \
          intermediate_code.c tac_generator.c optimize_tac_code.c assembly_generator.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = compiler

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

parser.tab.c parser.tab.h: parser.y
	$(BISON) -d parser.y

lex.yy.c: lexer.l parser.tab.h
	$(FLEX) lexer.l

main.o: main.c parser.tab.h ast.h intermediate_code.h symbol_table.h
	$(CC) $(CFLAGS) -c main.c -o main.o

lex.yy.o: lex.yy.c parser.tab.h
	$(CC) $(CFLAGS) -c lex.yy.c -o lex.yy.o

parser.tab.o: parser.tab.c ast.h symbol_table.h semantic_analysis.h intermediate_code.h tac_generator.h
	$(CC) $(CFLAGS) -c parser.tab.c -o parser.tab.o

ast.o: ast.c ast.h token_defs.h
	$(CC) $(CFLAGS) -c ast.c -o ast.o

symbol_table.o: symbol_table.c symbol_table.h
	$(CC) $(CFLAGS) -c symbol_table.c -o symbol_table.o

semantic_analysis.o: semantic_analysis.c semantic_analysis.h token_defs.h symbol_table.h
	$(CC) $(CFLAGS) -c semantic_analysis.c -o semantic_analysis.o

intermediate_code.o: intermediate_code.c intermediate_code.h token_defs.h
	$(CC) $(CFLAGS) -c intermediate_code.c -o intermediate_code.o

tac_generator.o: tac_generator.c tac_generator.h ast.h intermediate_code.h token_defs.h
	$(CC) $(CFLAGS) -c tac_generator.c -o tac_generator.o

optimize_tac_code.o: optimize_tac_code.c intermediate_code.h
	$(CC) $(CFLAGS) -c optimize_tac_code.c -o optimize_tac_code.o

assembly_generator.o: assembly_generator.c intermediate_code.h
	$(CC) $(CFLAGS) -c assembly_generator.c -o assembly_generator.o

clean:
	rm -f $(TARGET) $(OBJECTS) lex.yy.c parser.tab.c parser.tab.h

.PHONY: all clean