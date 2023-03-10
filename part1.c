/**
 * part1.c 
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
#define PAGE_MASK   0b00000000000011111111110000000000

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 0b00000000000000000000001111111111

#define MEMORY_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  unsigned int logical; // gives the same output as the sample if changed back to char, we changed it to int since page/frame numbers are 10 bits long
  unsigned int physical; // gives the same output as the sample if changed back to char, we changed it to int since page/frame numbers are 10 bits long
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

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(unsigned int logical_page) { // gives the same output as the sample if changed back to char, we changed it to int since page/frame numbers are 10 bits long
    /* TODO */
    // look at each element in tlb and return the physical page if its logical counterpart matches the input
    for (int i = 0; i < TLB_SIZE; i++){
      if (logical_page == tlb[i].logical){
        return tlb[i].physical;
      }
    }
    return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(unsigned int logical, unsigned int physical) { // gives the same output as the sample if changed back to char, we changed it to int since page/frame numbers are 10 bits long
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
  if (argc != 3) {
    fprintf(stderr, "Usage ./virtmem backingstore input\n");
    exit(1);
  }
  
  const char *backing_filename = argv[1]; 
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
  
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
          // go to the desired page in the backing store and copy its contents to the main memory using my_page as a temporary variable
          char my_page[PAGE_SIZE];
          FILE *backing_file = fopen(backing_filename, "r");
          fseek(backing_file, logical_page * PAGE_SIZE, SEEK_SET);
          fread(my_page, PAGE_SIZE, sizeof(char), backing_file);
          fclose(backing_file);
          //strncpy(&main_memory[free_page * PAGE_SIZE], my_page, PAGE_SIZE); // commented out since it did not copy after a null character is encountered
          for (int i = 0; i < PAGE_SIZE; i++){
            main_memory[free_page * PAGE_SIZE + i] = my_page[i];
          }
          // save the physical page number at the pagetable
          pagetable[logical_page] = free_page;
          // assign free_page to physical_page to be accessed later
          physical_page = free_page;
          // increment free_page to replace the next location in the memory next time a page fault occurs
          free_page++;
          // a page fault occured, increment page fault counter
          page_faults++;
      }

      add_to_tlb(logical_page, physical_page);
    }
    
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));
  
  return 0;
}