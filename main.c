#include "MEM.h"
#include "BCP.h"

int
main(int argc, char **argv)
{
    FILE *src_fp;
    MEM_Storage storage;

    storage = MEM_open_storage(4096);
    src_fp = fopen("name_test.b", "r");
    BCP_compile(storage, src_fp);

    return 0;
}
