

CFLAGS=-Wall -Werror -Wextra -pedantic -O3 -Wshadow

all: myshell

myshell : myshell.o lex.yy.o 
	cc $(CFLAGS) myshell.o lex.yy.o -o myshell -lfl

lex.yy.c :	lex.l
	flex lex.l

myshell.o: myshell.c
	cc $(CFLAGS) -c myshell.c

lex.yy.o: lex.yy.c
	cc -c lex.yy.c

clean:
	rm myshell lex.yy.o myshell.o lex.yy.c