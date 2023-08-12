#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline uintptr_t align_up(uintptr_t sz, size_t alignment)
{
    uintptr_t mask = alignment - 1;
    if ((alignment & mask) == 0) {  /* power of two? */
        // return MMMM;
        return ((sz + mask) & ~mask);
    }
    return (((sz + mask) / alignment) * alignment);
}

int main() 
{
    int size, align;
    while (scanf("%d %d", &size, &align)) {
        printf("align_up(%d, %d) = %d\n", size, align, align_up(size, align));
    }
    return 0;
}