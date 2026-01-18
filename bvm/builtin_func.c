#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "BVM.h"
#include "bvm_pri.h"

static FILE *st_current_input;
static FILE *st_current_output;

#define CHAR_IN_INT(array, n) (\
        ((array)[(n) / sizeof(int)] \
         >> ((sizeof(int) - 1 - ((n) % sizeof(int))) * CHAR_BIT)) & 0xff)

static void
put_str(FILE *fp, int *str)
{
    int ch;
    int i;

    for (i = 0; ; i++) {
        ch = CHAR_IN_INT(str, i);
        if (ch == BVM_EOT) {
            break;
        }
        fputc(ch, fp);
    }
}

static int
fun_printf(int argc, int *args, int *memory)
{
    int *fmt;
    int fmt_ch;
    int *values;
    int fmt_idx;
    int val_idx;

    fmt = &memory[args[0]];
    values = &args[1];
    val_idx = 0;

    for (fmt_idx = 0; ; fmt_idx++) {
        fmt_ch = CHAR_IN_INT(fmt, fmt_idx);
        if (fmt_ch == BVM_EOT) {
            break;
        }

        if (fmt_ch == '%') {
            if (CHAR_IN_INT(fmt, fmt_idx + 1) == 'c') {
                fprintf(st_current_output, "%c", values[val_idx]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == 'd') {
                fprintf(st_current_output, "%d", values[val_idx]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == 'o') {
                fprintf(st_current_output, "%o", values[val_idx]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == 's') {
                put_str(st_current_output, &memory[values[val_idx]]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == '%') {
                fputc('%', st_current_output);
            }
            fmt_idx++;
        } else {
            fputc(fmt_ch, st_current_output);
        }
    }
    return val_idx;
}

static int
fun_putnumb(int argc, int *args, int *memory)
{
    return fprintf(st_current_output, "%d", args[0]);
}

static int
fun_putchar(int argc, int *args, int *memory)
{
    int i;
    int ch;

    for (i = 0; i < sizeof(int); i++) {
        ch = (((unsigned int*)args)[0] >> ((sizeof(int) - i - 1)) * CHAR_BIT)
            & 0xff;
        if (ch != 0 && ch != BVM_EOT) {
            fputc(ch, st_current_output);
        } else if (ch == BVM_EOT) {
            break;
        }
    }

    return ch;
}

static int
fun_getchar(int argc, int *args, int *memory)
{
    int ret;
    ret = fgetc(st_current_input);

    if (ret == EOF) {
        return BVM_EOT;
    } else {
        return ret;
    }
}

static int
fun_char(int argc, int *args, int *memory)
{
    int s = args[0];
    int n = args[1];
    unsigned int y; /* word containing n-th char */
    int sh, cpos;
    
    y = ((unsigned int*)memory)[s + n / sizeof(int)];
    cpos = n % sizeof(int);        /* position of char in word */
    sh = (sizeof(int) - 1 - cpos) * CHAR_BIT; /* bit positions to shift */
    y =  (y >> sh) & 0xff;    /* shift and mask all but 8 bits */

   return y;         /* return character to caller */
}

static void
lchar(int s, int n, int c, int *memory)
{
    int y, cpos, sh, mask;

    y = memory[s + n / sizeof(int)];  /* word containing n-th char */
    cpos = n % sizeof(int);        /* position of char in word */
    sh = (sizeof(int) - 1 - cpos) * CHAR_BIT; /* bit positions to shift */
    mask = 0xff << sh;
    
    memory[s + n / sizeof(int)] =  (y & ~mask) | (c << sh);
}

static int
fun_lchar(int argc, int *args, int *memory)
{
    int s = args[0];
    int n = args[1];
    int c = args[2];

    lchar(s, n, c, memory);

    return c;
}

static int
fun_getstr(int argc, int *args, int *memory)
{
    int s = args[0];
    int c;
    int i;

    i = 0;
    while ((c = fgetc(st_current_input)) != '\n') {
        lchar(s, i++, c, memory);
    }
    lchar(s, i, BVM_EOT, memory);

    return s;
}

static int
fun_putstr(int argc, int *args, int *memory)
{
    int s = args[0];

    put_str(st_current_output, &memory[s]);

    return 0;
}

static int
fun_nargs(int argc, int *args, int *memory)
{
    return bvm_get_current_arg_count();
}

static int st_free_list = BVM_NULL;

static void
expand_heap_area(int size, int *memory)
{
    int alloc_size;
    int heap_end = bvm_get_heap_end_address();
    FreeBlock *fb;

    alloc_size = (size + HEAP_HEADER_SIZE) > HEAP_ALLOC_SIZE
        ? (size + HEAP_HEADER_SIZE) : HEAP_ALLOC_SIZE;
    bvm_set_heap_end_address(heap_end + alloc_size);
    fb = (FreeBlock*)&memory[heap_end];
    fb->size = alloc_size - HEAP_HEADER_SIZE;
    fb->next = st_free_list;
    st_free_list = heap_end;
}

static int
search_free_list(int size, int *memory)
{
    int pos;
    int prev;
    FreeBlock *pos_fb;
    int new_pos;
    FreeBlock *new_fb;
    FreeBlock *prev_fb;

    for (pos = st_free_list, prev = BVM_NULL; pos != BVM_NULL; ) {
        pos_fb = (FreeBlock*)&memory[pos];
        if (pos_fb->size >= size) {
            if (pos_fb->size >= size + HEAP_HEADER_SIZE) {
                new_pos = pos + size;
                new_fb = (FreeBlock*)&memory[new_pos];
                new_fb->size = pos_fb->size - size;
            } else {
                new_pos = BVM_NULL;
            }
            if (prev == BVM_NULL) {
                st_free_list = pos_fb->next;
            } else {
                prev_fb = (FreeBlock*)&memory[prev];
                prev_fb->next = pos_fb->next;
            }
            if (new_pos != BVM_NULL) {
                new_fb->next = st_free_list;
                st_free_list = new_pos;
            }
            return pos;
        }
        prev = pos;
        pos = pos_fb->next;
    }
    return BVM_NULL;
}

static int
fun_getvec(int argc, int *args, int *memory)
{
    int size = args[0] + 1;
    int ret;
    
    ret = search_free_list(size, memory);
    if (ret == BVM_NULL) {
        expand_heap_area(size, memory);
        ret = search_free_list(size, memory);
    }
    return ret;
}

static void
coalesce_free_block(int v, int size, int *memory)
{
    int pos;
    FreeBlock *pos_fb;
    int list_prev;
    int addr_prev = BVM_NULL;
    FreeBlock *addr_prev_fb;
    int addr_next = BVM_NULL;
    FreeBlock *addr_next_fb;
    int next_prev;
    FreeBlock *next_prev_fb;
    FreeBlock *v_fb;

    for (pos = st_free_list, list_prev = BVM_NULL; pos != BVM_NULL; ) {
        pos_fb = (FreeBlock*)&memory[pos];
        if (!((v + size <= pos) || (v >= pos + pos_fb->size))) {
            fprintf(stderr, "bad rlsevec.\n");
            exit(1);
        }
        if (pos + HEAP_HEADER_SIZE + pos_fb->size == v) {
            addr_prev = pos;
            addr_prev_fb = pos_fb;
        }
        if (pos == v + size) {
            addr_next = pos;
            addr_next_fb = pos_fb;
            next_prev = list_prev;
        }
        list_prev = pos;
        pos = pos_fb->next;
    }

    if (addr_prev != BVM_NULL && addr_next != BVM_NULL) {
        addr_prev_fb->size += size + HEAP_HEADER_SIZE + addr_next_fb->size;
        if (next_prev == BVM_NULL) {
            st_free_list = addr_next_fb->next;
        } else {
            next_prev_fb = (FreeBlock*)&memory[next_prev];
            next_prev_fb->next = addr_next_fb->next;
        }
    } else if (addr_prev != BVM_NULL) {
        addr_prev_fb->size += size;
    } else if (addr_next != BVM_NULL) {
        if (next_prev == BVM_NULL) {
            st_free_list = addr_next_fb->next;
        } else {
            next_prev_fb = (FreeBlock*)&memory[next_prev];
            next_prev_fb->next = addr_next_fb->next;
        }
        v_fb = (FreeBlock*)&memory[v];
        v_fb->size = size + addr_next_fb->size;
        v_fb->next = st_free_list;
        st_free_list = v;
    } else {
        if (size < HEAP_HEADER_SIZE) {
            return;
        }
        v_fb = (FreeBlock*)&memory[v];
        v_fb->size = size - HEAP_HEADER_SIZE;
        v_fb->next = st_free_list;
        st_free_list = v;
    }
}

static int
fun_rlsevec(int argc, int *args, int *memory)
{
    int v = args[0];
    int size = args[1] + 1;

    coalesce_free_block(v, size, memory);
    
    return 0;
}

static int
fun_dump_free_list(int argc, int *args, int *memory)
{
    int pos;
    FreeBlock *fb;
    int count = 0;

#if 0
    fprintf(st_current_output, "** HEAP START %d **\n",
            bvm_get_heap_start_address());
#endif
    for (pos = st_free_list; pos != BVM_NULL; ) {
        fb = (FreeBlock*)&memory[pos];
        
#if 0
        fprintf(st_current_output, "pos..%d, size..%d, next..%d\n",
                pos, fb->size, fb->next);
#endif
        count++;
        pos = fb->next;
    }
#if 0
    fprintf(st_current_output, "** HEAP END %d **\n",
            bvm_get_heap_end_address());
#endif
    return count;
}

static int
fun_exit(int argc, int *args, int *memory)
{
    exit(0);
}

static char *st_builtin_function_names[] = {
    "printf",
    "putnumb",
    "putchar",
    "getchar",
    "char",
    "lchar",
    "getstr",
    "putstr",
    "nargs",
    "getvec",
    "rlsevec",
    "dump_free_list",
    "exit",
};

BuiltinFunction
bvm_builtin_functions[] = {
    {"printf", fun_printf},
    {"putnumb", fun_putnumb},
    {"putchar", fun_putchar},
    {"getchar", fun_getchar},
    {"char", fun_char},
    {"lchar", fun_lchar},
    {"getstr", fun_getstr},
    {"putstr", fun_putstr},
    {"nargs", fun_nargs},
    {"getvec", fun_getvec},
    {"rlsevec", fun_rlsevec},
    {"dump_free_list", fun_dump_free_list},
    {"exit", fun_exit},
};

int
bvm_get_builtin_function_count(void)
{
    int count1 = sizeof(st_builtin_function_names) / sizeof(char*);
    int count2 = sizeof(bvm_builtin_functions) / sizeof(BuiltinFunction);
    
    assert(count1 == count2);

    return count1;
}

char **BVM_get_builtin_functions(int *count)
{
    *count = sizeof(st_builtin_function_names) / sizeof(char*);

    return st_builtin_function_names;
}

void
bvm_init_builtin_function(void)
{
    st_current_input = stdin;
    st_current_output = stdout;
}
