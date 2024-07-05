#include "trie.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "main.h"
#include "stdio.h"

struct trie_node *root;

void trie_init()
{
    root = (trie_node *)malloc(sizeof(trie_node));
    root->c = 0;
    root->term = 0;
    root->subwords = 0;
    root->parent = NULL;
    for (int i = 0; i < LETTERS; i++)
    {
        root->children[i] = NULL;
    }
    pthread_mutex_init(&root->lock, NULL);
}

void trie_add_word(char *word)
{
    struct trie_node *curr = root;
    pthread_mutex_lock(&curr->lock);

    int i = 0;
    int added = 1;
    while (word[i] && i < MAX_WORD_LEN - 1)
    {

        char c = word[i];
        if (c >= 'A' && c <= 'Z')
        {
            c += 'a' - 'A';
        }
        if (curr->children[c - 'a'] == NULL)
        {
            added = 0;
            struct trie_node *new = (trie_node *)malloc(sizeof(trie_node));
            new->c = c;
            new->term = 0;
            new->subwords = 0;
            new->parent = curr;
            for (int i = 0; i < LETTERS; i++)
            {
                new->children[i] = NULL;
            }
            pthread_mutex_init(&new->lock, NULL);

            curr->children[c - 'a'] = new;
        }

        struct trie_node *next = curr->children[c - 'a'];
        pthread_mutex_lock(&next->lock);
        pthread_mutex_unlock(&curr->lock);
        curr = next;
        i++;
    }

    curr->term = 1;
    struct trie_node *prev = curr->parent;
    pthread_mutex_unlock(&curr->lock);
    curr = prev;

    if (added)
        return;

    while (curr->parent != NULL)
    {
        pthread_mutex_lock(&curr->lock);
        curr->subwords++;
        prev = curr->parent;
        pthread_mutex_unlock(&curr->lock);
        curr = prev;
    }
    add_result(word);
}

int find_words_rek(int i, int curr_word_count, char **words, char *buf, trie_node *curr)
{
    int prev_word_count = curr_word_count;
    buf[i] = curr->c;
    buf[i + 1] = '\0';

    if (curr->term)
    {
        words[curr_word_count] = (char *)malloc(MAX_WORD_LEN * sizeof(char));
        strncpy(words[curr_word_count], buf, MAX_WORD_LEN);
        words[curr_word_count][MAX_WORD_LEN - 1] = '\0';
        curr_word_count++;
    }

    for (int k = 0; k < LETTERS; k++)
    {
        if (curr->children[k] == NULL)
            continue;
        curr_word_count += find_words_rek(i + 1, curr_word_count, words, buf, curr->children[k]);
    }
    return curr_word_count - prev_word_count;
}

search_result *trie_get_words(char *prefix)
{
    trie_node *curr = root;
    pthread_mutex_lock(&curr->lock);

    search_result *result = (search_result *)malloc(sizeof(search_result));
    int i = 0;

    while (prefix[i] != '\n' && prefix[i] != ' ' && prefix[i] != '\t' && curr != NULL && i < MAX_WORD_LEN - 1)
    {
        char c = tolower(prefix[i]);
        trie_node *next = curr->children[c - 'a'];
        if (next != NULL)
        {
            pthread_mutex_lock(&next->lock);
        }
        pthread_mutex_unlock(&curr->lock);
        curr = next;
        i++;
    }

    if (curr == NULL)
    {
        result->result_count = 0;
        return result;
    }

    result->result_count = curr->subwords;
    result->words = (char **)malloc(result->result_count * MAX_WORD_LEN);

    char buf[MAX_WORD_LEN];
    i = 0;
    while (prefix[i] != '\n')
    {
        buf[i] = prefix[i];
        i++;
    }
    buf[i] = '\0';

    int curr_word_count = 0;
    for (int k = 0; k < LETTERS; k++)
    {
        if (curr->children[k] == NULL)
            continue;
        curr_word_count += find_words_rek(i, curr_word_count, result->words, buf, curr->children[k]);
    }
    pthread_mutex_unlock(&curr->lock);
    return result;
}

void trie_free_result(search_result *result)
{
    for (int i = 0; i < result->result_count; i++)
    {
        if (result->words[i] == NULL)
            continue;
        free(result->words[i]);
    }
    free(result->words);
    free(result);
}