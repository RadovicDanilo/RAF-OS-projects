#include "trie.h"
#include "scanner.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

#define MAX_COMMAND_LEN 512

search_result *result;
pthread_mutex_t resut_lock;
char *prefix;

void *autocomplete(void *_args)
{
    prefix = (char *)_args;
    int i = 0;
    result = trie_get_words(prefix);
    while (1)
    {
        pthread_mutex_lock(&resut_lock);
        if (result->result_count == 0)
        {
            printf("No terms found.\n");
            return 0;
        }
        while (i < result->result_count)
        {
            printf("%s\n", result->words[i]);
            i++;
        }
        pthread_mutex_unlock(&resut_lock);

        pthread_testcancel();
    }
}

void add_result(char *term)
{
    if (prefix[0] == '\0')
        return;
    if (strncmp(prefix, term, strlen(prefix) - 1) != 0)
        return;
    if (result == NULL)
        return;

    pthread_mutex_lock(&resut_lock);
    result->result_count++;
    result->words = realloc(result->words, result->result_count * sizeof(char *));
    result->words[result->result_count - 1] = malloc(MAX_WORD_LEN * sizeof(char));
    strncpy(result->words[result->result_count - 1], term, MAX_WORD_LEN);
    pthread_mutex_unlock(&resut_lock);
}

void scan(char *command)
{
    pthread_t scanner_thread;

    char *path = malloc(128);
    path[0] = '.';
    path[1] = '/';
    for (int i = 6;; i++)
    {
        if (command[i] == '\n')
        {
            path[i - 4] = '\0';
            break;
        }
        path[i - 4] = command[i];
    }

    pthread_create(&scanner_thread, NULL, scanner_work, path);
}

int main()
{
    prefix = malloc(MAX_WORD_LEN);
    prefix[0] = '\0';

    pthread_mutex_init(&resut_lock, NULL);

    trie_init();
    scanner_init();

    while (1)
    {
        char command[MAX_COMMAND_LEN] = {'\0'};
        fgets(command, MAX_COMMAND_LEN, stdin);

        if (strncmp("_add_", command, 5) == 0)
        {
            scan(command);
        }
        else if (strncmp("_stop_", command, 6) == 0)
        {
            break;
        }
        else
        {
            pthread_t reader_thread;
            pthread_create(&reader_thread, NULL, autocomplete, command);

            char *ctrl_d = malloc(MAX_COMMAND_LEN);
            while (1)
            {
                if (fgets(ctrl_d, MAX_COMMAND_LEN, stdin) == NULL)
                {
                    if (feof(stdin))
                    {
                        clearerr(stdin);
                        break;
                    }
                }
            }
            pthread_cancel(reader_thread);
            pthread_join(reader_thread, NULL);

            trie_free_result(result);
            prefix[0] = '\0';
            free(ctrl_d);
        }

        printf("\n");
    }

    pthread_mutex_destroy(&resut_lock);
    exit(0);
}