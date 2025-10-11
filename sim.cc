#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <algorithm>

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/

const char WRITE_COM = 'w';
const char READ_COM = 'r';

int mem_traffic = 0;

void update_lru(Cache* LX, uint32_t index, uint32_t prev, uint32_t addr) {
   int flag = 0;
   for (uint32_t i = 0; i < LX->ASSOC; i++) {
      if (LX->sets[index][i].LRU < prev) {
         LX->sets[index][i].LRU++;
      }
      else if (flag == 0 && LX->sets[index][i].LRU == prev) {
         LX->sets[index][i].LRU = 0;
         flag++;
      }
   }
}

void sort_vec(Cache* LX) {
   for (uint32_t i = 0; i < LX->nums_sets; i++) {
      for (uint32_t j = 1; j < LX->ASSOC; j++) {
         for (uint32_t k = 0; k < LX->ASSOC; k++) {
            if (LX->sets[i][k].LRU > LX->sets[i][j].LRU && LX->sets[i][k].valid && LX->sets[i][j].valid) {
            Mem_Space* temp = new Mem_Space();
            Mem_Space::copy_mem_space(temp, &LX->sets[i][k]);
            Mem_Space::copy_mem_space(&LX->sets[i][k], &LX->sets[i][j]);
            Mem_Space::copy_mem_space(&LX->sets[i][j], temp);
            
            }
         }
      }
   }
}

uint32_t find_MRU(Cache* LX, uint32_t index) {
   uint32_t max = 0;
   uint32_t element = 0;
   for (uint32_t i = 0; i < LX->ASSOC; i++) {
      if (LX->sets[index][i].LRU > max) {
         max = LX->sets[index][i].LRU;
         element = i;
      }
   }
   return element;
}

uint32_t command_new(Cache* LX, uint32_t address, char read_write, bool write_back) {
   uint32_t index = 0;
   uint32_t tag = 0;
   if (LX->nums_sets == 0) {
      index = 0;
      tag = address >> LX->nums_block_offset;
   }
   else 
   {
      index = (address >> LX->nums_block_offset) & ((1<< LX->nums_index) - 1);
      tag = address >> (LX->nums_index + LX->nums_block_offset);
   }
   //uint32_t index = (address >> LX->nums_block_offset) & ((1<< LX->nums_index) - 1);
   //uint32_t tag = address >> (LX->nums_index + LX->nums_block_offset);
   uint32_t res_addr = address;

   for (uint32_t i = 0; i < LX->ASSOC; i++) {
      // L1 or L2 hit
      if (LX->sets[index][i].value == tag) {
         if (write_back) {
            LX->sets[index][i].address = address;
            LX->sets[index][i].value = tag;
            LX->sets[index][i].dirty = true;
            LX->write++;
            //if (read_write != READ_COM) LX->write++;
            update_lru(LX, index, LX->sets[index][i].LRU, LX->sets[index][i].value);
            return 1;
         }
         
         if (read_write == READ_COM) {
            LX->read++;
         }
         else {
            //if (LX->next_cache == nullptr) LX->read++;
            LX->sets[index][i].dirty = true;
            LX->sets[index][i].address = address;
            LX->sets[index][i].value = tag;

            LX->write++;
         }

         update_lru(LX, index, LX->sets[index][i].LRU, LX->sets[index][i].value);
         return address;
      }
   }

   // write back and regular recursive call handled for L1 here
   uint32_t MRU = find_MRU(LX, index);
   
   if (LX->next_cache != nullptr) {
      
      if (LX->sets[index][MRU].dirty) {
         
         //reconstruct address
         // dirty write back to L2
         LX->write_back++;
         write_back = true;
         res_addr = command_new(LX->next_cache, LX->sets[index][MRU].address, WRITE_COM, write_back);
         LX->sets[index][MRU].dirty = false;
         //LX->next_cache->write++;
         //handle eviction for L1 dirty
      }

      write_back = false;
      res_addr = command_new(LX->next_cache, address, READ_COM, write_back);

      if (res_addr == address) {
         // LX->next_cache->read++;
         //printf("L1 evicted address/tag:  tag: %x LRU: %d Dirty bit: %d\n", LX->sets[index][MRU].value, LX->sets[index][MRU].LRU, LX->sets[index][MRU].dirty);
         LX->sets[index][MRU].value = tag;
         LX->sets[index][MRU].address = address;
         if (read_write == READ_COM) {
            LX->sets[index][MRU].dirty = false;
            LX->read++;
            LX->read_miss++;
            //LX->next_cache->write++;
         }
         else {
            LX->sets[index][MRU].dirty = true;
            LX->write++;
            LX->write_miss++;
         }
         
         update_lru(LX, index, LX->sets[index][MRU].LRU, LX->sets[index][MRU].value);
         return 1;
      }
   }

   // handle L2 write back and L2 miss here
   //either need to fetch from main memory
   // add write back
   //mem_traffic++;
   if (LX->sets[index][MRU].dirty && LX->next_cache == nullptr) {
      LX->write_back++;
      mem_traffic++;
      //LX->read_miss++;
   }

   uint32_t MRU_addr = LX->sets[index][MRU].address;

   if (read_write == READ_COM) {
      LX->sets[index][MRU].dirty = false;
      LX->read++;
      LX->read_miss++;
   }
   else {
      LX->sets[index][MRU].dirty = true;
      LX->write++;
      LX->write_miss++;
   }

   if (LX->next_cache == nullptr) {
      mem_traffic++;
   }

   LX->sets[index][MRU].valid = true;
   LX->sets[index][MRU].value = tag;
   LX->sets[index][MRU].address = address;

   update_lru(LX, index, LX->sets[index][MRU].LRU, LX->sets[index][MRU].value);

   return MRU_addr;
}

int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   printf("\n");

   Cache* L1 = new Cache(params.BLOCKSIZE, params.L1_SIZE, params.L1_ASSOC);
   //printf("%d\n%d\n%d\n",L1->BLOCKSIZE, L1->SIZE, L1->ASSOC);
   //if (L1->next_cache == nullptr) printf("NULLPTR");

   Cache* L2 = new Cache();
   //Cache* L2 = new Cache(params.BLOCKSIZE, params.L2_SIZE, params.L2_ASSOC);
   //if (L1->next_cache == nullptr) printf("NULLPTR");
   //printf("%d\n%d\n%d\n",L2->BLOCKSIZE, L2->SIZE, L2->ASSOC);

   if (static_cast<int>(params.L2_SIZE) > 0) {
      L2 = new Cache(params.BLOCKSIZE, params.L2_SIZE, params.L2_ASSOC);
      L1->next_cache = L2;
   }

   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
   //    if (rw == 'r') {
   //       printf("r %x\n", addr);
         
   //       // printf("%d\n", read_command(*L1.next_cache,addr));
   //    }
   //    else if (rw == 'w') {
   //        printf("w %x\n", addr);
   //    }
   //    else {
   //       printf("Error: Unknown request type %c.\n", rw);
	//  exit(EXIT_FAILURE);
   //    }

      ///////////////////////////////////////////////////////
      // Issue the request to the L1 cache instance here.
      ///////////////////////////////////////////////////////
      //read_command(L1,addr,rw);
      command_new(L1, addr, rw, false);
    }
    
    
    //Mem_Space* temp = new Mem_Space();
    printf("===== L1 contents =====\n");
    for (uint32_t i = 0; i < L1->nums_sets; i++) {
      //sort(L1->sets[i].begin(), L1->sets[i].end(),compare_LRU);
      sort_vec(L1);
      printf("set      %d:    ",i);
      for (uint32_t j = 0; j < L1->ASSOC; j++) {
         printf("%x", L1->sets[i][j].value);
         if (L1->sets[i][j].dirty) {
            printf(" D");
         }
         if (L1->ASSOC == j + 1) printf("\n");
         else printf("   ");
      }
    }

    if (L2->SIZE > 0) {
      printf("\n===== L2 contents =====\n");
      for (uint32_t i = 0; i < L2->nums_sets; i++) {
         // sort(L2->sets[i].begin(), L2->sets[i].end(), [](const Mem_Space &a, const Mem_Space &b){
         //    return a.LRU < b.LRU;
         // });
         sort_vec(L2);
         //printf("tag: %x lru: %d || tag: %x lru: %d || tag: %x lru: %d || tag: %x lru: %d ||",L2->sets[i][0].value,L2->sets[i][0].LRU,L2->sets[i][1].value,L2->sets[i][1].LRU,L2->sets[i][2].value,L2->sets[i][2].LRU,L2->sets[i][3].value,L2->sets[i][3].LRU);
         //sort_vec(L2);
         printf("set      %d:    ", i);
         for (uint32_t j = 0; j < L2->ASSOC; j++) {
            
            printf("%x", L2->sets[i][j].value);
            if (L2->sets[i][j].dirty) {
               printf(" D");
            }
            if (L2->ASSOC == j + 1) printf("\n");
            else printf("   ");
         }
      }
    }
    printf("\n===== Measurements =====\n");
    printf("a. L1 reads:                   %d\n", L1->read);
    printf("b. L1 read misses:             %d\n", L1->read_miss);
    printf("c. L1 writes:                  %d\n", L1->write);
    printf("d. L1 write misses:            %d\n", L1->write_miss);
    printf("e. L1 miss rate:               %.4f\n", static_cast<float>(L1->read_miss + L1->write_miss)/(L1->read + L1->write));
    printf("f. L1 writebacks:              %d\n", L1->write_back);
    printf("g. L1 prefetches:              %d\n", L1->prefetches);

    printf("h. L2 reads (demand):          %d\n", L2->read);
    printf("i. L2 read misses (demand):    %d\n", L2->read_miss);
    printf("j. L2 reads (prefetch):        0\n");
    printf("k. L2 read misses (prefetch):  0\n");
    printf("l. L2 writes:                  %d\n", L2->write);
    printf("m. L2 write misses:            %d\n", L2->write_miss);
    if (L2->read > 0 || L2->write > 0) {
      printf("n. L2 miss rate:               %.4f\n", static_cast<float>(L2->read_miss)/L2->read);
    }
    else {
      printf("n. L2 miss rate:               0.0000\n");
    }
   //  printf("L2 miss rate: %.2d\n", (L2->read_miss + L2->write_miss)/(L2->read + L2->write));
    printf("o. L2 writebacks:              %d\n", L2->write_back);
    printf("p. L2 prefetches:              %d\n", L2->prefetches);

    printf("q. memory traffic:             %d\n", mem_traffic);

    delete(L1);
    delete(L2);
    return(0);
}
