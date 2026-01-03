#include "MEM.h"
#include "BCP.h"
#include "BVM.h"

int
main(int argc, char **argv)
{
    FILE *src_fp;
    int main_address;
    int heap_start_address;
    int *memory;

    if (argc != 2) {
        fprintf(stderr, "usage: %s filename.b", argv[0]);
    }

    src_fp = fopen(argv[1], "r");

    memory = BCP_compile(src_fp, &main_address, &heap_start_address);
    BVM_execute(memory, main_address, heap_start_address);

    return 0;
}
