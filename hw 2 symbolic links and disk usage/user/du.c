#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char buf[512];

char *fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

int total = 0;
int file = 0;

void du(char *path, int flag)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_NOFOLLOW)) < 0)
    {
        fprintf(2, "du: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "du: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch (st.type)
    {
    case T_FILE:
        printf("%s %d\n", fmtname(path), st.blocks);
        break;

    case T_SYMLINK:
        printf("%s %d\n", fmtname(path), st.blocks);
        break;

    case T_DIR:
        file = 1;
        if (flag)
        {
            printf("%s %d\n", fmtname(path), st.blocks);
            total++;
        }
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("du: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            if (stat(buf, &st) < 0)
            {
                printf("du: cannot stat %s\n", buf);
                continue;
            }
            if (fmtname(buf)[0] == '.' || fmtname(buf)[1] == '.')
                continue;

            if (st.type == T_DIR)
            {
                du(buf, 0);
            }
            printf("%s %d\n", fmtname(buf), st.blocks);
            total += st.blocks;
        }

        break;
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    int i;
    total = 0;
    file = 0;
    if (argc < 2)
    {
        du(".", 1);
        if (file)
            printf("%s %d\n", "total:", total);
        exit();
    }
    for (i = 1; i < argc; i++)
        du(argv[i], 1);
    if (file)
        printf("%s %d\n", "total:", total);
    exit();
}
