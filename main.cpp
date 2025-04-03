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
    std::uniform_int_distribution<int> dist(1, nframes); 

    int victim_page = -1;
    int victim_frame = -1;
    int victim_bits = 0;

    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    int rand_frame = dist(gen);
    cout << "Generated random frame: " << rand_frame << endl;

    // Loop through page_mapping to find the page that maps to rand_frame
    for (int i = 0; i < pt->npages; ++i) {
        if (pt->page_mapping[i] == rand_frame && pt->page_bits[i] != PROT_NONE) {
            victim_page = i;
            victim_frame = rand_frame;
            victim_bits = pt->page_bits[i];
            break;
        }
    }

    if (victim_page != -1) {// valid victim page is found, evict it
        cout << "Evicting page #" << victim_page << " from frame #" << victim_frame << " with bits " << victim_bits << endl;

        // Handle dirty page (write to disk)
        if (victim_bits == (PROT_READ | PROT_WRITE)) {
            printf("Dirty victim page, writing to disk\n");
            disk_write(disk, victim_frame, pt->physmem + victim_frame * PAGE_SIZE);
        }

        page_table_set_entry(pt, victim_page, 0, PROT_NONE);

        cout << "Loading new page #" << page << " into frame #" << victim_frame << endl;
        disk_read(disk, page, pt->physmem + victim_frame * PAGE_SIZE);
        page_table_set_entry(pt, page, rand_frame, PROT_READ);  

    } else {// No victim page found, likely because the frame is not in use
        cout << "No page found to evict in frame " << rand_frame << endl;

        cout << "Loading new page #" << page << " into frame #" << rand_frame << endl;
        disk_read(disk, page, pt->physmem + rand_frame * PAGE_SIZE);
        page_table_set_entry(pt, page, rand_frame, PROT_READ);  
    }

    cout << "After ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}


// Handler Wrapper
void page_fault_handler(struct page_table *pt, int page) {
    // get cur permissions
    int frame, bits;
    page_table_get_entry(pt, page, &frame, &bits);
    cout << "Handler called on page #" << page << " frame " << frame <<" Bits " << bits <<endl;


    if (bits == PROT_NONE) {
        // If the page is not in memory, bring it in using random_replace
        cout << "page fault on page #" << page <<endl;
        random_replace(pt, page);
        
        // After bringing it into memory, set the permissions to READ
        cout << "Page #" << page << " is now in memory with READ permissions." << endl;
    }
    else if (bits == PROT_READ) {// If the page is in memory with READ permissions, change to READ/WRITE
        cout << "Page #" << page << " already in memory with READ permissions, upgrading to READ/WRITE." << endl;
        page_table_set_entry(pt, page, frame, PROT_READ | PROT_WRITE);
    }
    else {// We should never encounter a page with RW permissions, as it would be evicted to NONE
        cerr << "ERROR: Invalid page state for page #" << page << ": Unexpected permissions!" << endl;
        exit(1);
    }
}


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
    struct page_table *pt = page_table_create(npages, nframes, page_fault_handler);
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
