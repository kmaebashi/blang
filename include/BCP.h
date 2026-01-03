#ifndef BCP_H_INCLUDED
#define BCP_H_INCLUDED
#include "MEM.h"

int *BCP_compile(FILE *src_fp, int *main_address, int *heap_start_address);

#endif /* BCP_H_INCLUDED */
