#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        symlink(0, 0);
        exit();
    }
    if (symlink(argv[1], argv[2]) == -1)
    {
        printf("error symlink\n");
    }
    exit();
}
