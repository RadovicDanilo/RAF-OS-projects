#include "types.h"
#include "param.h"
#include "defs.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

struct shm
{
    char name[MAXNAMESHM];
    uint ref;
    uint sz;
    char *pages[MAXSHMSZ];
};
