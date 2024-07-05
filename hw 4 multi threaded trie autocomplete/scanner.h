#include "time.h"

typedef struct scanned_file // datoteka koju je scanner vec skenirao
{
    char file_name[256]; // naziv datoteke
    time_t mod_time;     // vreme poslednje modifikacije datoteke
} scanned_file;

extern void scanner_init();             // poziva se jednom na pocetku rada sistema
extern void *scanner_work(void *_args); // funkcija scanner niti