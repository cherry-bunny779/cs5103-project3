
/*
Do not modify this file.
Make all of your changes to main.cpp instead.
*/

#include "page_table.h"

#include <iomanip>
#include <iostream>

#include <fcntl.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/mman.h>

using std::cerr;
using std::cout;
using std::endl;

// To avoid warning
int remap_file_pages(void *addr, size_t size, int prot,
                     size_t pgoff, int flags);

struct page_table *the_page_table = 0;

static void internal_fault_handler(int signum, siginfo_t *info, void *context)
{

#ifdef i386
    char *addr = (char *)(((struct ucontext *)context)->uc_mcontext.cr2);
#else
    char *addr = (char *)info->si_addr;
#endif

    struct page_table *pt = the_page_table;

    if (pt)
    {
        int page = (addr - pt->virtmem) / PAGE_SIZE;

        if (page >= 0 && page < pt->npages)
        {
            pt->handler(pt, page);
            return;
        }
    }

    cerr << "segmentation fault at address " << addr << endl;
    abort();
}

struct page_table *page_table_create(int npages, int nframes, page_fault_handler_t handler)
{
    int i;
    struct sigaction sa;
    struct page_table *pt;
    char filename[256];

    pt = new struct page_table;
    if (!pt)
        return 0;

    the_page_table = pt;

    sprintf(filename, "/tmp/pmem.%d.%d", getpid(), getuid());

    pt->fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0777);
    if (!pt->fd)
        return 0;

    ftruncate(pt->fd, PAGE_SIZE * npages);

    unlink(filename);

    pt->physmem = (char *)mmap(0, nframes * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pt->fd, 0);
    pt->nframes = nframes;

    pt->virtmem = (char *)mmap(0, npages * PAGE_SIZE, PROT_NONE, MAP_SHARED | MAP_NORESERVE, pt->fd, 0);
    pt->npages = npages;

    pt->page_bits = new int[npages];
    pt->page_mapping = new int[npages];

    pt->handler = handler;

    for (i = 0; i < pt->npages; i++)
        pt->page_bits[i] = 0;

    sa.sa_sigaction = internal_fault_handler;
    sa.sa_flags = SA_SIGINFO;

    sigfillset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, 0);

    return pt;
}

void page_table_delete(struct page_table *pt)
{
    munmap(pt->virtmem, pt->npages * PAGE_SIZE);
    munmap(pt->physmem, pt->nframes * PAGE_SIZE);
    delete[] pt->page_bits;
    delete[] pt->page_mapping;
    close(pt->fd);
    delete pt;
}

void page_table_set_entry(struct page_table *pt, int page, int frame, int bits)
{
    if (page < 0 || page >= pt->npages)
    {
        cerr << "page_table_set_entry: illegal page #" << page << endl;
        abort();
    }

    if (frame < 0 || frame >= pt->nframes)
    {
        cerr << "page_table_set_entry: illegal frame #" << frame << endl;
        abort();
    }

    pt->page_mapping[page] = frame;
    pt->page_bits[page] = bits;

    remap_file_pages(pt->virtmem + page * PAGE_SIZE, PAGE_SIZE, 0, frame, 0);
    mprotect(pt->virtmem + page * PAGE_SIZE, PAGE_SIZE, bits);
}

void page_table_get_entry(struct page_table *pt, int page, int *frame, int *bits)
{
    if (page < 0 || page >= pt->npages)
    {
        cerr << "page_table_get_entry: illegal page #" << page << endl;
        abort();
    }

    *frame = pt->page_mapping[page];
    *bits = pt->page_bits[page];
}

void page_table_print_entry(struct page_table *pt, int page)
{
    if (page < 0 || page >= pt->npages)
    {
        cerr << "page_table_print_entry: illegal page #" << page << endl;
        abort();
    }

    int b = pt->page_bits[page];

    cout << "page " << std::setfill('0') << std::setw(6) << page;
    cout << ": frame " << std::setfill('0') << std::setw(6) << pt->page_mapping[page];
    cout << " bits ";
    cout << ((b & PROT_READ) ? 'r' : '-');
    cout << ((b & PROT_WRITE) ? 'w' : '-');
    cout << ((b & PROT_EXEC) ? 'x' : '-');
    cout << endl;
}

void page_table_print(struct page_table *pt)
{
    int i;
    for (i = 0; i < pt->npages; i++)
    {
        page_table_print_entry(pt, i);
    }
}

int page_table_get_nframes(struct page_table *pt)
{
    return pt->nframes;
}

int page_table_get_npages(struct page_table *pt)
{
    return pt->npages;
}

char *page_table_get_virtmem(struct page_table *pt)
{
    return pt->virtmem;
}

char *page_table_get_physmem(struct page_table *pt)
{
    return pt->physmem;
}
