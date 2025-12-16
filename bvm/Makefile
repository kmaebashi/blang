TARGET = bvm.o
CFLAGS = -c -g -DDEBUG -Wall -ansi -pedantic -Wswitch-enum
OBJS = \
  ./opcode_info.o \
  ./execute.o \
  ./builtin_func.o

$(TARGET):$(OBJS)
	ld -r -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $*.c -I../include
############################################################
builtin_func.o: builtin_func.c bvm_pri.h
execute.o: execute.c ../include/BVM.h
opcode_info.o: opcode_info.c ../include/BVM.h
