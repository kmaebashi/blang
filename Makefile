SUBDIRS = memory compiler
OBJS = \
  ./main.o\
  compiler/compiler.o\
  memory/mem.o
CFLAGS = -c -g -DDEBUG -Wall -ansi -pedantic -Wswitch-enum
TARGET = lexer

.PHONY: all $(SUBDIRS)

bcomp: $(SUBDIRS) main.o
	$(CC) $(OBJS) -o $@ -lm

$(SUBDIRS):
	$(MAKE) -C $@

.c.o:
	$(CC) $(CFLAGS) $*.c -I./include
############################################################
main.o: main.c include/MEM.h include/BCP.h include/MEM.h
