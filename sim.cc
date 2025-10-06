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
const char EMPTY_SET = 'e';
const char WRITE_COM = 'w';
const char READ_COM = 'r';
const char WRITE_HIT = '1';
const char READ_HIT = '2';
const char WRITE_MISS = '3';
const char READ_MISS = '4';
int mem_traffic = 0;

bool compare_LRU(const Mem_Space &block1, const Mem_Space &block2) {
      return block1.LRU < block2.LRU;
    }

void update_lru(Cache* LX, uint32_t index, int prev, uint32_t addr = 0) {
   if (prev < 0) {
      for (uint32_t i = 0; i < LX->ASSOC; i++) {
         if (LX->sets[index][i].value != addr) LX->sets[index][i].LRU++;
      }
   }
   for (uint32_t i = 0; i < LX->ASSOC; i++) {
      if (static_cast<int>(LX->sets[index][i].LRU) < prev) LX->sets[index][i].LRU++;
      else if (static_cast<int>(LX->sets[index][i].LRU) == prev) LX->sets[index][i].LRU = 0;
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

// void eviction(Cache LX, uint32_t address) {
   
// }

// write back write allocate
// uint32_t write_command(Cache* LX, uint32_t address, char read_write) {
//    uint32_t index = (address >> LX->nums_block_offset) & ((1<< LX->nums_index) - 1);
//    uint32_t tag = address >> (LX->nums_index + LX->nums_block_offset);
//    int prev_lru = 0;
//    uint32_t res_addr = address;

//    for (uint32_t i = 0; i < LX->ASSOC; i++) {
      
//       if (LX->sets[index][i].value == tag) {
//          prev_lru = i;
//          update_lru(LX, index, prev_lru);
//          LX->write++;
//          if (LX->next_cache == nullptr) {
//             L1_write_miss++;
//             printf("in L2\n");
//          }
//          else {
//             LX->sets[index][i].dirty = true;
//             LX->sets[index][i].valid = true;
//             LX->sets[index][i].value = tag;
//          }
//          return address;
//       }
//    }

//    if (LX.next_cache != nullptr) {
//       LX.write_miss++;
//       res_addr = write_command(*LX.next_cache, address, read_write);
//       if (res_addr == address) {
//          uint32_t MRU = find_MRU(LX, index);
//          uint32_t MRU_address = LX.sets[index][MRU].value;
         
//          LX.sets[index][MRU].value = tag;
//          LX.sets[index][MRU].valid = true;
//          LX.sets[index][MRU].dirty = true;
//          prev_lru = -1;
//          update_lru(LX, index, prev_lru);
//          return MRU_address;
//       }
//    }

//    return 1;
// }

// uint32_t reconstruct_addr(uint32_t index, uint32_t tag) {

// }

uint32_t command(Cache* LX, uint32_t address, char read_write, bool write_back) {
   uint32_t index = (address >> LX->nums_block_offset) & ((1<< LX->nums_index) - 1);
   uint32_t tag = address >> (LX->nums_index + LX->nums_block_offset);
   int prev_lru = 0;
   uint32_t res_addr = address;

   for (uint32_t i = 0; i < LX->ASSOC; i++) {
      // L1 or L2 hit
      if (LX->sets[index][i].value == tag) {
         // prev_lru = i;
         // update_lru(LX, index, prev_lru);
         if (write_back) {
            //printf("WRITEBACK");
            LX->sets[index][i].address = address;
            LX->sets[index][i].dirty = true;
            return 1;
         }
         prev_lru = i;
         update_lru(LX, index, prev_lru);
         if (read_write == READ_COM) {
            //if (LX->next_cache != nullptr) LX->sets[index][i].dirty = false;
            LX->read++;
         }
         else {
            //if (LX->next_cache != nullptr) LX->sets[index][i].dirty = true;
            LX->sets[index][i].dirty = true;
            LX->write++;
         }
         return address;
      }
   }

   // write back and regular recursive call handled for L1 here
   uint32_t MRU = find_MRU(LX, index);
   //printf("CHECK FOR IF L1");
   if (LX->next_cache != nullptr) {
      //printf("IN L1 still going to check dirty");
      if (LX->sets[index][MRU].dirty) {
         //printf("AT L1, MISS");
         //reconstruct address
         LX->write_back++;
         write_back = true;
         res_addr = command(LX->next_cache, LX->sets[index][MRU].address, read_write, write_back);
         //handle eviction for L1 dirty
      }

      write_back = false;
      res_addr = command(LX->next_cache, address, read_write, false);

      if (res_addr == address) {
         LX->sets[index][MRU].value = tag;
         LX->sets[index][MRU].address = address;
         if (read_write == READ_COM) {
            LX->sets[index][MRU].dirty = false;
            LX->read++;
            LX->read_miss++;
         }
         else {
            LX->sets[index][MRU].dirty = true;
            LX->write++;
            LX->write_miss++;
            //printf("write miss");
         }
         LX->sets[index][MRU].LRU = 0;

         prev_lru = -1;
         update_lru(LX, index, prev_lru, tag);
         return address;
      }
   }

   // handle L2 write back and L2 miss here
   //either need to fetch from main memory
   // add write back
   //mem_traffic++;
   if (LX->sets[index][MRU].dirty) {
      LX->write_back++;
      mem_traffic++;
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
      //printf("write miss");
   }

   //LX->sets[index][MRU].dirty = false;
   mem_traffic++;
   LX->sets[index][MRU].valid = true;
   LX->sets[index][MRU].value = tag;
   LX->sets[index][MRU].address = address;
   prev_lru = -1;
   LX->sets[index][MRU].LRU = 0;
   update_lru(LX, index, prev_lru, tag);

   return MRU_addr;
}

uint32_t read_command(Cache* LX, uint32_t address, char read_write) {
   uint32_t index = (address >> LX->nums_block_offset) & ((1<< LX->nums_index) - 1);
   uint32_t tag = address >> (LX->nums_index + LX->nums_block_offset);
   int prev_lru = 0;
   uint32_t res_addr = address;

   for (uint32_t i = 0; i < LX->ASSOC; i++) {
      // L1 or L2 hit
      if (LX->sets[index][i].value == tag) {
         prev_lru = i;
         update_lru(LX, index, prev_lru);
         if (read_write == READ_COM) {
            //if (LX->next_cache != nullptr) LX->sets[index][i].dirty = false;
            LX->read++;
         }
         else {
            if (LX->next_cache != nullptr) LX->sets[index][i].dirty = true;
            LX->write++;
         }

         // dirty 
         // if (LX->next_cache != nullptr) {
         //    LX->sets[index][i].dirty = true;
         //    LX->sets[index][i].valid = true;
         //LX->sets[index][i].value = tag;
         // }
         return address;
      }
   }


   // miss L1
   if (LX->next_cache != nullptr) {
      uint32_t MRU = find_MRU(LX, index);
      // if (LX->sets[index][MRU].dirty) {
      //    uint32_t MRU_address = LX->sets[index][MRU].address;

      // }
      //LX->read_miss++;
      res_addr = read_command(LX->next_cache, address, read_write);
      // hit L2 no evition @ L2
      // evict L1
      if (res_addr == address) {
         // find the LRU
         //uint32_t MRU = find_MRU(LX, index);
         //uint32_t MRU_addr = LX->sets[index][MRU].value;

         if (read_write == READ_COM) {
            //LX->sets[index][MRU].dirty = false;
            LX->read++;
            LX->read_miss++;
         }
         else {
            //LX->sets[index][MRU].dirty = true;
            LX->write++;
            LX->write_miss++;
         }

         LX->sets[index][MRU].address = address;
         LX->sets[index][MRU].value = tag;
         //LX->sets[index][MRU].valid = true;
         LX->sets[index][MRU].LRU = 0;

         // update LRU count
         prev_lru = -1;
         update_lru(LX, index, prev_lru, tag);
         return 1;
      }
      else {
         LX->write_back++;
         //need to evict and clean
         for (uint32_t i = 0; i < LX->ASSOC; i++) {
            if (LX->sets[index][i].value == res_addr) {
               // if (!LX->sets[index][i].dirty) {
               //    LX->sets[index][i].value = tag;
               //    LX->sets[index][i].LRU = 0;
               // }
               if (read_write == READ_COM) {
                  LX->read++;
                  LX->read_miss++;
                  //LX->sets[index][i].dirty = false;
               }
               else {
                  LX->write++;
                  LX->write_miss++;
                  //LX->sets[index][i].dirty = true;
               }

               LX->sets[index][i].address = address;
               LX->sets[index][i].value = tag;
               LX->sets[index][i].LRU = 0;
               //LX->sets[index][i].valid = true;
               prev_lru = -1;
               update_lru(LX, index, prev_lru, address);
            }
         }
         return 1;
      }
   }

   // miss L2
   // clean dirty writeback bit, clean L2
   //printf("in L2\n");
   // for (uint32_t i = 0; i < LX->ASSOC; i++) {
   //    if (LX->sets[index][i].value == res_addr) {
   //       LX->sets[index][i].dirty = false;
   //    }
   // }

   // evict and write in bit
   //LX->read_miss++;
   // if (LX->next_cache == nullptr) L2_read_miss++;
   //else L1_read_miss++;
   uint32_t MRU = find_MRU(LX, index);
   uint32_t MRU_addr = LX->sets[index][MRU].value;

   if (read_write == READ_COM) LX->read_miss++;
   else LX->write_miss++;
   
   mem_traffic++;
   LX->sets[index][MRU].value = tag;
   LX->sets[index][MRU].dirty = false;
   LX->sets[index][MRU].valid = true;
   LX->sets[index][MRU].LRU = 0;
   LX->write_back++;

   prev_lru = -1;
   update_lru(LX, index, prev_lru, address);
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
      
      // Cache L2 = Cache(params.BLOCKSIZE, params.L2_SIZE, params.L2_ASSOC);
      // printf("%d\n%d\n%d\n",L2.BLOCKSIZE, L2.SIZE, L2.ASSOC);
      delete(L2);
      Cache* L2 = new Cache(params.BLOCKSIZE, params.L2_SIZE, params.L2_ASSOC);
      L1->next_cache = L2;
   }

   // printf("%d\n",L1.next_cache->ASSOC);
   //print_func(*L1->next_cache);

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
      command(L1, addr, rw, false);
    }

    
    //Mem_Space* temp = new Mem_Space();
    printf("===== L1 contents =====\n");
    for (uint32_t i = 0; i < L1->nums_sets; i++) {
      sort(L1->sets[i].begin(), L1->sets[i].end(),compare_LRU);
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
         // for (uint32_t k = 1; k < L2->ASSOC; k++) {
         //    if (L2->sets[i][k - 1].LRU > L2->sets[i][k].LRU) {
         //       temp = L2->sets[]
         //    }
         // }
         sort(L2->sets[i].begin(), L2->sets[i].end(), compare_LRU);
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
    printf("e. L1 miss rate:               %.6f\n", static_cast<double>(L1->read_miss + L1->write_miss)/(L1->read + L1->write));
    printf("f. L1 writebacks:              %d\n", L1->write_back);
    printf("g. L1 prefetches:              %d\n", L1->prefetches);

    printf("h. L2 reads (demand):          %d\n", L2->read);
    printf("i. L2 read misses (demand):    %d\n", L2->read_miss);
    printf("j. L2 reads (prefetch):        0\n");
    printf("k. L2 read misses (prefetch):  0\n");
    printf("l. L2 writes:                  %d\n", L2->write);
    printf("m. L2 write misses:            %d\n", L2->write_miss);
    if (L2->read > 0 || L2->write > 0) {
      printf("n. L2 miss rate:               %.6f\n", static_cast<double>(L2->read_miss + L2->write_miss)/(L2->read + L2->write));
    }
    else {
      printf("n. L2 miss rate:               0\n");
    }
   //  printf("L2 miss rate: %.2d\n", (L2->read_miss + L2->write_miss)/(L2->read + L2->write));
    printf("o. L2 writebacks:              %d\n", L2->write_back);
    printf("p. L2 prefetches:              %d\n", L2->prefetches);

    printf("q. memory traffic:             %d\n", mem_traffic);

    delete(L1);
    delete(L2);
    return(0);
}
