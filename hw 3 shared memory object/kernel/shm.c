#include "shm.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "fcntl.h"
#include "proc.h"

struct shm shm_table[NSHM];
struct spinlock shm_lock;

void shm_init()
{
    initlock(&shm_lock, "shm_lock");
    for (int i = 0; i < NSHM; i++)
    {
        for (int j = 0; j < MAXNAMESHM; j++)
        {
            shm_table[i].name[j] = 0;
        }
        shm_table[i].ref = 0;
        shm_table[i].sz = 0;
        for (int j = 0; j < MAXSHMSZ; j++)
        {
            shm_table[i].pages[j] = 0;
        }
    }
}

int shm_open(char *name)
{
    acquire(&shm_lock);
    int j = NSHM;
    for (int i = 0; i < NSHM; i++)
    {
        if (shm_table[i].ref == 0)
        {
            j = min(j, i);
            continue;
        }
        if (strncmp(shm_table[i].name, name, strlen(name)) != 0)
            continue;
        if (proc_shm_isopened(i) != -1)
        {
            release(&shm_lock);
            return i;
        }
        if (proc_shm_add(i) < 0)
        {
            release(&shm_lock);
            return -1;
        }
        shm_table[i].ref++;
        release(&shm_lock);
        return i;
    }

    if (j == NSHM)
    {
        release(&shm_lock);
        return -1;
    }

    if (proc_shm_isopened(j) != -1)
    {
        release(&shm_lock);
        return j;
    }

    if (proc_shm_add(j) < 0)
    {
        release(&shm_lock);
        return -1;
    }

    strncpy(&shm_table[j].name[0], name, strlen(name));
    shm_table[j].ref++;
    release(&shm_lock);
    return j;
}

int shm_trunc(int shm_od, int size)
{
    if (shm_od < 0 || shm_od >= NSHM || size <= 0 || proc_shm_isopened(shm_od) < 0)
        return -1;

    acquire(&shm_lock);
    if (shm_table[shm_od].ref == 0)
    {
        release(&shm_lock);
        return -1;
    }

    if (shm_table[shm_od].sz > 0)
    {
        release(&shm_lock);
        return -1;
    }
    int pages = (size + PGSIZE - 1) / PGSIZE;
    if (pages > MAXSHMSZ)
    {
        release(&shm_lock);
        return -1;
    }
    for (int i = 0; i < pages; i++)
    {
        char *page = kalloc();
        if (page)
        {
            memset(page, 0, PGSIZE);
            shm_table[shm_od].pages[i] = page;
            continue;
        }
        continue;
        for (int j = 0; j < i; j++)
        {
            kfree(shm_table[shm_od].pages[j]);
            shm_table[shm_od].pages[j] = 0;
        }
        release(&shm_lock);
        return -1;
    }
    shm_table[shm_od].sz = pages;

    release(&shm_lock);
    return pages * PGSIZE;
}

int shm_map(int shm_od, void **va, int flags)
{
    if (shm_od < 0 || shm_od >= NSHM)
        return -1;
    if (proc_shm_isopened(shm_od) < 0 || proc_shm_ismapped(shm_od) == 0)
        return -1;
    if (flags != O_RDWR && flags != O_RDONLY)
        return -1;

    acquire(&shm_lock);
    if (shm_table[shm_od].sz == 0)
    {
        release(&shm_lock);
        return -1;
    }

    struct proc *p = myproc();

    void *adr = (void *)(KERNBASE / 2 + p->shm_top * PGSIZE);

    for (int i = 0; i < shm_table[shm_od].sz; i++)
    {
        if (map_shm_pages(p->pgdir, adr + i * PGSIZE, PGSIZE, V2P(shm_table[shm_od].pages[i]), flags | PTE_U) < 0)
        {
            release(&shm_lock);
            return -1;
        }
    }

    p->shm_adr[proc_shm_isopened(shm_od)] = adr;
    *va = adr;
    p->shm_top += shm_table[shm_od].sz;

    release(&shm_lock);
    return 0;
}

int shm_close(int shm_od)
{
    if (shm_od < 0 || shm_od >= NSHM || proc_shm_isopened(shm_od) < 0)
    {
        return -1;
    }
    acquire(&shm_lock);

    shm_table[shm_od].ref--;

    if (proc_shm_close(shm_od, shm_table[shm_od].sz) < 0)
    {
        release(&shm_lock);
        return -1;
    }
    if (shm_table[shm_od].ref > 0)
    {
        release(&shm_lock);
        return 0;
    }

    for (int j = 0; j < MAXNAMESHM; j++)
    {
        shm_table[shm_od].name[j] = 0;
    }

    for (int j = 0; j < shm_table[shm_od].sz; j++)
    {
        if (shm_table[shm_od].pages[j] == 0)
            continue;
        kfree(shm_table[shm_od].pages[j]);
        shm_table[shm_od].pages[j] = 0;
    }

    shm_table[shm_od].sz = 0;
    release(&shm_lock);
    return 0;
}

// za debug
int shm_info(shm_od)
{
    if (shm_od < 0 || shm_od >= NSHM)
        return -1;

    acquire(&shm_lock);
    struct shm shm = shm_table[shm_od];
    cprintf("shm_od = %d, name = %s, ref = %d, size = %d\n", shm_od, shm.name, shm.ref, shm.sz);
    release(&shm_lock);
    return 0;
}

int shm_increment_ref(int shm_od)
{
    if (shm_od < 0 || shm_od >= NSHM)
        return -1;
    acquire(&shm_lock);
    shm_table[shm_od].ref++;
    release(&shm_lock);
    return 0;
}
