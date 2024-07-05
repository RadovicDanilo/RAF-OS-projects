#include "param.h"
#include "pthread.h"

typedef struct trie_node // cvor unutar trie strukture
{
    char c;                              // slovo ovog cvora
    int term;                            // flag za kraj reci
    int subwords;                        // broj reci u podstablu, ne racunajuci sebe
    struct trie_node *parent;            // pokazivac ka roditelju
    struct trie_node *children[LETTERS]; // deca
    pthread_mutex_t lock;
} trie_node;

typedef struct search_result // rezultat pretrage
{
    int result_count; // duzina niza
    char **words;     // niz stringova, svaki string duzine MAX_WORD_LEN
} search_result;

extern void trie_init();                             // poziva se jednom na pocetku rada sistema
extern void trie_add_word(char *word);               // operacija za dodavanje reci
extern search_result *trie_get_words(char *prefix);  // operacija za pretragu
extern void trie_free_result(search_result *result); // rezultat se dinamicki alocira pri pretrazi, tako da treba da postoji funkcija za oslobadjanje tog rezultata