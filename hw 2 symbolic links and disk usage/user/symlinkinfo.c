#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char *
fmtname(char *path)
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

char buf[512];

void cat(int fd)
{
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        if (write(1, buf, n) != n)
        {
            printf("cat: write error\n");
            exit();
        }
        printf("\n");
    }
    if (n < 0)
    {
        printf("cat: read error\n");
        exit();
    }
}

void symlinkinfo(char *path)
{
    char buf[512], *p;
    int fd, fd2;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_NOFOLLOW)) < 0)
    {
        fprintf(2, "symlinkinfo: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "symlinkinfo: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch (st.type)
    {

    case T_SYMLINK:
        printf("%s ->", fmtname(path));
        cat(fd);
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("symlinkinfo: path too long\n");
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

                printf("symlinkinfo: cannot stat %s\n", buf);
                continue;
            }
            if (st.type != T_SYMLINK)
            {
                continue;
            }
            printf("%s -> ", fmtname(buf));
            fd2 = open(buf, O_NOFOLLOW);
            cat(fd2);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        symlinkinfo(".");
        exit();
    }
    for (i = 1; i < argc; i++)
    {

        symlinkinfo(argv[i]);
    }
    exit();
}
