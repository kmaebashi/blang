#include <stdio.h>
#include "bvm_pri.h"

int
bvm_fun_printf(int *args, int *memory)
{
    char *fmt;
    int *values;
    int fmt_idx;
    int val_idx;

    fmt = (char*)&memory[args[0]];
    values = &args[1];
    val_idx = 0;

    for (fmt_idx = 0; fmt[fmt_idx] != '\0'; fmt_idx++) {
        if (fmt[fmt_idx] == '%') {
            if (fmt[fmt_idx + 1] == 'c') {
                printf("%c", values[val_idx]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == 'd') {
                printf("%d", values[val_idx]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == 'o') {
                printf("%o", values[val_idx]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == 's') {
                printf("%s", (char*)&memory[values[val_idx]]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == '%') {
                putchar('%');
            }
            fmt_idx++;
        } else {
            putchar(fmt[fmt_idx]);
        }
    }
    return val_idx;
}

#if 0
int
bvm_fun_print(int *args, int *memory)
{
    printf("%d\n", args[0]);

    return 8;
}

int
bvm_fun_print_s(int *args, int *memory)
{
    printf("%s\n", (char*)&memory[args[0]]);

    return 15;
}
#endif
