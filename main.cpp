/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <cassert>
#include <iostream>
#include <string.h>
#include <random>
#include <unistd.h>

using namespace std;

// Prototype for test program
typedef void (*program_f)(char *data, int length);

// Number of physical frames
int nframes;
int npages;
int last_victim,last_frame,evicted_page;
bool flag = false;
// Pointer to disk for access from handlers
struct disk *disk = nullptr;

// Simple handler for pages == frames
void page_fault_handler_example(struct page_table *pt, int page)
{
    cout << "page fault on page #" << page << endl;

    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    // Map the page to the same frame number and set to read/write
    // TODO - Disable exit and enable page table update for example
    //exit(1);
    page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);
    
    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}
// 1. bookeeping frames used 2. handler function that decides which replacement policy

void random_replace(struct page_table *pt, int page) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, npages - 1);

    int victim_page = dist(gen);  // Choose a random page to evict

    int current_frame, current_bits, re_frame;
    cout << "page fault on page #" << page << " "  << victim_page  << " " << current_frame << " " << current_bits <<endl;
    page_table_get_entry(pt, victim_page, &current_frame, &current_bits);
    
    cout << "entry to evict" << page << " "  << victim_page  << " " << current_frame << " " << current_bits <<endl;

    // make frame
    if(npages >= nframes){
        re_frame = page%nframes;
    } else{
        re_frame = page;
    }
    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
    printf("current_bits: %i\n",current_bits);
    if (current_bits == 0x01) // catch write operation
    {
        printf("branch write taken\n");
        page_table_print_entry(pt, page);
        page_table_set_entry(pt, page, current_frame, PROT_READ | PROT_WRITE);
        page_table_print_entry(pt, page);
        
    }
    else if (current_bits == 0x11)  // evict dirty
    {
        printf("branch evict taken\n");
        page_table_print_entry(pt, victim_page);
        page_table_set_entry(pt, victim_page, current_frame, PROT_NONE);
        disk_write(disk,current_frame,pt->physmem + current_frame*PAGE_SIZE);
        disk_read(disk,current_frame,pt->physmem + page*PAGE_SIZE);
        page_table_set_entry(pt, page, current_frame, PROT_READ);
        page_table_print_entry(pt, victim_page);
    }
    else
    {
        page_table_set_entry(pt, page%nframes, re_frame, PROT_READ);
    }

    //page_table_get_entry(pt, victim_page, &frame, &bits);
    
    // Evict victim page and load new page into the same frame
    // page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);
    printf("currentpage: %i\n", page);

    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}


// TODO - Handler(s) and page eviction algorithms

int main(int argc, char *argv[])
{
    // Check argument count
    if (argc != 5)
    {
        cerr << "Usage: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>" << endl;
        exit(1);
    }

    // Parse command line arguments
    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *algorithm = argv[3];
    const char *program_name = argv[4];

    // Validate the algorithm specified
    if ((strcmp(algorithm, "rand") != 0) &&
        (strcmp(algorithm, "fifo") != 0) &&
        (strcmp(algorithm, "custom") != 0))
    {
        cerr << "ERROR: Unknown algorithm: " << algorithm << endl;
        exit(1);
    }

    // Validate the program specified
    program_f program = NULL;
    if (!strcmp(program_name, "sort"))
    {
        if (nframes < 2)
        {
            cerr << "ERROR: nFrames >= 2 for sort program" << endl;
            exit(1);
        }

        program = sort_program;
    }
    else if (!strcmp(program_name, "scan"))
    {
        program = scan_program;
    }
    else if (!strcmp(program_name, "focus"))
    {
        program = focus_program;
    }
    else
    {
        cerr << "ERROR: Unknown program: " << program_name << endl;
        exit(1);
    }

    // TODO - Any init needed

    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk)
    {
        cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << endl;
        return 1;
    }

    // Create a page table
    struct page_table *pt = page_table_create(npages, nframes, random_replace);
    if (!pt)
    {
        cerr << "ERROR: Couldn't create page table: " << strerror(errno) << endl;
        return 1;
    }

    // Run the specified program
    char *virtmem = page_table_get_virtmem(pt);
    program(virtmem, npages * PAGE_SIZE);

    // Clean up the page table and disk
    page_table_delete(pt);
    disk_close(disk);

    return 0;
}
