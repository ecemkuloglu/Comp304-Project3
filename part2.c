/**
 * virtmem.c 
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
#define PAGE_MASK 1047552/* TODO */

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 1023/* TODO */

#define MEMORY_SIZE FRAMES * PAGE_SIZE
#define FRAMES 256

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  unsigned char logical;
  unsigned char physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

//keeping counter for each page, then searching for the page with highest counter and elimina
int pagetable_counter[PAGES];

void initiate_LRU_counter() {
	for(int i=0; i<PAGES; i++) pagetable_counter[i]=0;
}

void increment_LRU_counter() {
	for(int i=0; i<PAGES; i++) pagetable_counter[i]++;
}

int search_LRU_counter() {
	int max=0;
	for(int i=0; i<PAGES; i++){
		if(pagetable_counter[i]>max) max=pagetable_counter[i];
	}
	return max;		
}
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
int search_tlb(unsigned char logical_page) {
    /* TODO */
    int size = TLB_SIZE;
    while(size != 0) { // search TLB
      if (logical_page == tlb[size].logical) { //check the virtual addresses if they exist 
       return tlb[size].physical; //returns the physical address
      }
      size--;
    }
    return -1; //returns -1 if virtual address is not present on TLB
    
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(unsigned char logical, unsigned char physical) {
    /* TODO */
    int index = tlbindex % TLB_SIZE; // from line 33 use as tlbindex % TLB_SIZE for the index of the next TLB line to use
    tlb[index].logical = logical;
    tlb[index].physical = physical;
    tlbindex++;
}



int main(int argc, const char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage ./virtmem backingstore input -p 0/1\n");
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
  
  int fifo_or_lru = atoi(argv[4]); //take arg to decide fifo or lru

  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];
  
  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;
  
  // Number of the next unallocated physical page in main memory
  unsigned char free_page = 0;
  
  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
    total_addresses++;
    int logical_address = atoi(buffer);
    
    initiate_LRU_counter();
    /* TODO 
    / Calculate the page offset and logical page number from logical_address */
    int offset = logical_address & OFFSET_MASK;
    int logical_page = logical_address & PAGE_MASK;
    logical_page = logical_address >> 10;
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
          page_faults++;
          physical_page = free_page; //allocating new physical memory address from the available backing storage
          memcpy(main_memory + free_page * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);     
          pagetable[logical_page] = physical_page; //mapping the new physical address to the logical address
         
          if (page_faults == FRAMES) {
            if(fifo_or_lru == 0){
              free_page++;
              free_page = free_page % FRAMES;
            }
            else if (fifo_or_lru == 1){
              //do lru things
                free_page = search_LRU_counter();
                physical_page = free_page;
                pagetable_counter[free_page] = 0;
                pagetable[logical_address]=physical_page;
                increment_LRU_counter();
            }
          }else {
            free_page++; //incrementing free page number to update the pointer for backing storage for further use
          }
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
