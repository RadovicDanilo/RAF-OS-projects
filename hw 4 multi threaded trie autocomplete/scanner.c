#include "trie.h"
#include "scanner.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "sys/stat.h"
#include "dirent.h"
#include "unistd.h"

#define MAX_THREADS 128
#define SLEEP_DURATION 5
#define MAX_FILES 512

struct pthread_t *threads[MAX_THREADS];

struct scanned_file *scanned_files;
int scanned_files_count;

void scanner_init()
{
    scanned_files_count = 0;
    scanned_files = (scanned_file *)malloc(MAX_FILES * sizeof(scanned_file));
}

char *make_path_string(char *s1, char *s2)
{
    char *path = malloc(strlen(s1) + strlen(s2) + 2);
    int i = 0;
    int j = 0;

    while (s1[j] != 0)
    {
        path[i] = s1[j];
        i++;
        j++;
    }
    path[i] = '/';
    i++;
    j = 0;
    while (s2[j] != 0)
    {
        path[i] = s2[j];
        i++;
        j++;
    }
    path[i] = '\0';
    return path;
}

void scan_file(FILE *file)
{
    char word[MAX_WORD_LEN];
    int i = 0;

    while (1)
    {
        char c = fgetc(file);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            if (i == MAX_WORD_LEN - 1)
            {
                word[i] = 0;
                i = 0;
                trie_add_word(word);
            }
            else
            {
                word[i] = c;
                i++;
            }
        }
        else if (c == ' ' || c == '\t' || c == '\n')
        {
            if (i == 0)
                continue;
            word[i] = 0;
            trie_add_word(word);
            i = 0;
        }
        else if (c == EOF)
        {
            if (i == 0)
                break;
            word[i] = 0;
            trie_add_word(word);
            break;
        }
        else
        {
            i = 0;
        }
    }
}

void scan_directory(char *dir_path)
{
    DIR *dir = opendir(dir_path);
    FILE *file;
    struct dirent *dirent;
    if (dir == NULL)
        return;
    while ((dirent = readdir(dir)) != NULL)
    {
        if (dirent->d_type != 8)
            continue;

        char *path = make_path_string(dir_path, dirent->d_name);

        struct stat buffer;
        if (stat(path, &buffer) != 0)
        {
            continue;
        }
        int flag = 0;
        for (int i = 0; i < scanned_files_count; i++)
        {
            if (strcmp(scanned_files[i].file_name, path) != 0)
                continue;
            if (scanned_files[i].mod_time == buffer.st_mtime)
            {
                flag = 1;
            }
            else
            {
                scanned_files[i].mod_time = buffer.st_mtime;
                flag = 2;
            }
            break;
        }

        if (flag == 1)
            continue;
        if (flag == 0)
        {
            scanned_file *sf = (scanned_file *)malloc(sizeof(scanned_file));
            strcpy(sf->file_name, path);
            sf->mod_time = buffer.st_mtime;
            scanned_files[scanned_files_count] = *sf;
            scanned_files_count++;
        }

        file = fopen(path, "r");
        if (!file)
        {
            return;
        }
        scan_file(file);
        fclose(file);
    }
}

void *scanner_work(void *_args)
{
    char *dir_path = _args;
    while (1)
    {
        scan_directory(dir_path);
        sleep(SLEEP_DURATION);
    }
}