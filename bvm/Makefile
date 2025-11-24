TARGET = bvm.o
CFLAGS = -c -g -DDEBUG -Wall -ansi -pedantic -Wswitch-enum
OBJS = \
  ./opcode_info.o

$(TARGET):$(OBJS)
	ld -r -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $*.c -I../include
############################################################
opcode_info.o: opcode_info.c ../include/BVM.h
