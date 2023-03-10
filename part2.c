/**
 * part2.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024
#define FRAMES 256
#define PAGE_MASK   0b00000000000011111111110000000000

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 0b00000000000000000000001111111111

#define MEMORY_SIZE FRAMES * PAGE_SIZE
#define BACKING_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  unsigned int logical;
  unsigned int physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

// 1 if using LRU, 0 if using FIFO
int using_lru = -1;

// higher value in the lru array means less recently used
int lru[FRAMES];

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(unsigned int logical_page) {
  // look at each element in tlb and return the physical page if its logical counterpart matches the input
    /* TODO */
    for (int i = 0; i < TLB_SIZE; i++){
      if (logical_page == tlb[i].logical){
        return tlb[i].physical;
      }
    }
    return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(unsigned int logical, unsigned int physical) {
    /* TODO */
    // swap out the entry at the circularly accessed tlbindex with the input
    struct tlbentry my_entry;
    my_entry.logical = logical;
    my_entry.physical = physical;
    tlb[tlbindex % TLB_SIZE] = my_entry;
    tlbindex++;
}

int main(int argc, const char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage ./virtmem backingstore input -p 0/1\n");
    exit(1);
  }

  if (strcmp(argv[3], "-p")){
    fprintf(stderr, "Usage ./virtmem backingstore input -p 0/1\n");
    fprintf(stderr, "3rd argument should be -p\n");
    exit(1);
  }

  if (strcmp(argv[4], "0") && strcmp(argv[4], "1")){
    fprintf(stderr, "Usage ./virtmem backingstore input -p 0/1\n");
    fprintf(stderr, "4th argument should be 0 or 1\n");
    exit(1);
  }

  if (!strcmp(argv[4], "0")){
    using_lru = 0;
  }
  else if (!strcmp(argv[4], "1")){
    using_lru = 1;
  }
  
  const char *backing_filename = argv[1]; 
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, BACKING_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
  
  const char *input_filename = argv[2];
  FILE *input_fp = fopen(input_filename, "r");
  
  // Fill page table entries with -1 for initially empty table.
  int i;
  for (i = 0; i < PAGES; i++) {
    pagetable[i] = -1;
  }
  // Fill tlb entries with -1 for initially empty tlb
  for (i = 0; i < TLB_SIZE; i++){
    tlb[i].logical = -1;
    tlb[i].physical = -1;
  }
  
  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];
  
  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;
  int lru_on = 0;
  int lrused;
  
  // Number of the next unallocated physical page in main memory
  unsigned int free_page = 0;
  
  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
    total_addresses++;
    int logical_address = atoi(buffer);

    /* TODO 
    / Calculate the page offset and logical page number from logical_address */
    int offset = logical_address & OFFSET_MASK; // offset is given at the rightmost 10 bits
    int logical_page = (logical_address & PAGE_MASK) >> OFFSET_BITS; // page number is given at the rightmost 10th to 19th bits, shift to right by OFFSET_BITS to get the actual value
    ///////
    
    int physical_page = search_tlb(logical_page);
    // TLB hit
    if (physical_page != -1) {
      tlb_hits++;
      // TLB miss
    } else {
      physical_page = pagetable[logical_page];
      
      // Page fault
      if (physical_page == -1) {
          /* TODO */
          // no need to use page replace ment if there are free pages available in the memory
          if (free_page >= FRAMES){
            if (!using_lru){ // FIFO
              // swap out main_memory[(free_page % FRAMES)*PAGE_SIZE]
              // make it unavailable in tlb and pagetable
              // to_remove_physical is free_page % FRAMES
              int to_remove_logical;
              for (int i = 0; i < PAGES; i++){ // unavailable at pagetable
                if ((free_page % FRAMES) == pagetable[i]){ // free_page % FRAMES shows the first address to be put into the memory
                  to_remove_logical = i; // logical page corresponding to free_page % FRAMES is to be removed
                  pagetable[i] = -1; // make it unavailable at the page table
                  break;
                }
              }
              for (int i = 0; i < TLB_SIZE; i++){ // unavailable at tlb
                if (to_remove_logical == tlb[i].logical){ // search for the logical page to be removed in the tlb
                  tlb[i].physical = -1; // set its physical value to -1 (unavailable)
                  break;
                }
              }

            }
            else { // LRU
              // set lru_on to 1 to switch from filling the memory to replacing the least recently used ones
              lru_on = 1;
              int to_remove_logical;
              int max_uses = -1;
              lrused = -1;
              for (int i = 0; i < FRAMES; i++){ // find the frame with the highest lru value (the least recently used one)
                if (lru[i] >= max_uses){
                  max_uses = lru[i];
                  lrused = i; // frame to be replaces
                }
              }
              for (int i = 0; i < PAGES; i++){ // unavailable at pagetable
                if ((lrused) == pagetable[i]){ // lrused shows the first address to be put into the memory
                  to_remove_logical = i; // logical page corresponding to lrused is to be removed
                  pagetable[i] = -1; // make it unavailable at the page table
                  break;
                }
              }
              for (int i = 0; i < TLB_SIZE; i++){ // unavailable at tlb
                if (to_remove_logical == tlb[i].logical){ // search for the logical page to be removed in the tlb
                  tlb[i].physical = -1; // set its physical value to -1 (unavailable)
                  break;
                }
              }
            }
          }
          // go to the desired page in the backing store and copy its contents to the main memory using my_page as a temporary variable
          char my_page[PAGE_SIZE];
          FILE *backing_file = fopen(backing_filename, "r");
          fseek(backing_file, logical_page * PAGE_SIZE, SEEK_SET);
          fread(my_page, PAGE_SIZE, sizeof(char), backing_file);
          fclose(backing_file);
          if (!lru_on){
            //strncpy(&main_memory[(free_page % FRAMES) * PAGE_SIZE], my_page, PAGE_SIZE); // commented out since it did not copy after a null character is encountered
            for (int i = 0; i < PAGE_SIZE; i++){
              main_memory[(free_page % FRAMES) * PAGE_SIZE + i] = my_page[i];
            }
            // save the physical page number at the pagetable by circularly indexing the memory
            pagetable[logical_page] = free_page % FRAMES;
            // assign free_page to physical_page to be accessed later by circularly indexing the memory
            physical_page = free_page % FRAMES;
          }
          else {
            // put the new page in place of the least recently used one
            for (int i = 0; i < PAGE_SIZE; i++){
              main_memory[(lrused) * PAGE_SIZE + i] = my_page[i];
            }
            // save the physical page number at the pagetable
            pagetable[logical_page] = lrused;
            // assign lrused to physical_page to be accessed later
            physical_page = lrused;
          }
          // increment free_page to replace the next location in the memory next time a page fault occurs
          free_page++;
          // a page fault occured, increment page fault counter
          page_faults++;
      }

      add_to_tlb(logical_page, physical_page);
    }
    
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    // increment the lru value of all addresses to "age" them
    for (int i = 0; i < FRAMES; i++){
      lru[i]++;
    }
    // set the lru value of the most recently used address
    lru[physical_page] = 0;
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));
  
  return 0;
}
