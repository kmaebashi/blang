TARGET = compiler.o
CFLAGS = -c -g -DDEBUG -Wall -ansi -pedantic -Wswitch-enum
OBJS = \
  ./parser.o\
  ./lexer.o\
  ./fix_tree.o\
  ./dump_tree.o\
  ./gencode.o\
  ./message.o

$(TARGET):$(OBJS)
	ld -r -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $*.c -I../include
############################################################
dump_tree.o: dump_tree.c compiler.h ../include/BCP.h ../include/MEM.h \
 token.h
fix_tree.o: fix_tree.c ../include/BVM.h compiler.h ../include/BCP.h \
 ../include/MEM.h token.h
gencode.o: gencode.c ../include/BVM.h compiler.h ../include/BCP.h \
 ../include/MEM.h token.h
lexer.o: lexer.c ../include/MEM.h token.h compiler.h ../include/BCP.h \
 ../include/MEM.h
message.o: message.c compiler.h ../include/BCP.h ../include/MEM.h token.h
parser.o: parser.c compiler.h ../include/BCP.h ../include/MEM.h token.h
