name=MyJS
libs=
CFLAGS=-Wall -Wextra -ansi -fsanitize=address $(libs)
SRCS=$(wildcard src/*.c)
OBJSC=$(SRCS:src/%.c=bin/%.o)
OBJSL=$(SRCS:src/%.l=bin/%.o)
OBJSY=$(SRCS:src/%.y=bin/%.o)
OBJS=$(OBJSC) $(OBJSL) $(OBJSY)
GEN_OBJS=$(GEN_SRCS:%.c=%.o)
CC=gcc
YACC	= bison --yacc
YFLAGS	= -d
LEX	= flex
LFLAGS	=

main: CFLAGS+=-pedantic -Werror
main: $(name)

debug: CFLAGS+=-g
debug: $(name)

$(name): $(OBJS) $(GEN_OBJS)
	$(CC) $(OBJSC) $(GEN_OBJS) $(CFLAGS) -o $(name) 

bin/%.o: src/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/y.tab.c: src/*.y | bin
	$(YACC) $(YFLAGS) -b bin/y $<
	mv bin/y.tab.c bin/y.tab.h bin/

bin/lex.yy.c: src/*.l | bin
	$(LEX) $(LFLAGS) -o $@ $<

bin/lex.yy.o: bin/lex.yy.c bin/y.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/y.tab.o: bin/y.tab.c
	$(CC) $(CFLAGS) -c $< -o $@


bin:
	mkdir -p bin

clear:
	if ! [ -d "bin/" ] ; then \
		mkdir bin; \
	fi; \
	if ! [ -z "$$(ls -A ./bin 2>/dev/null)" ] ; then \
		rm bin/*; \
	fi; \
	if [ -f $(name)* ] ; then \
		rm $(name)*; \
	fi;

debug-srcs:
	@echo "SRCS: $(SRCS)"
	@echo "OBJS: $(OBJS)"
