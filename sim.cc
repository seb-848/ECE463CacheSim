#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"

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
int L1_read = 0;
int L1_read_miss = 0;
int L1_write = 0;
int L1_write_miss = 0;
int L1_miss_rate = 0;
int L1_write_back = 0;
int L1_prefetches = 0;
int L2_read = 0;
int L2_read_miss = 0;
int L2_write = 0;
int L2_write_miss = 0;
int L2_miss_rate = 0;
int L2_write_back = 0;
int L2_prefetches = 0;


// compare for lru_update
// bool compare_lru(, Mem_Space addr2) {
//    return addr1.LRU < addr2.LRU;
// }


// LRU Update
// void lru_update(Cache &LX, uint32_t index, int prev) {
//    int update = 0;
//    sort(LX.sets[index].begin(), LX.sets[index].end());
//    for (int i = 1; i <= prev; i++) {
//       lx.sets[index][i];
//    }
//    // for (int i = 0; i < LX.ASSOC; i++) {
//    //    switch(update) {
//    //       case 0:
//    //       if (LX.sets[index][i].value == tag) {

//    //       }
//    //       break;
//    //       case 1;
//    //       break;
//    //    }
//    // }
// }

void update_lru(Cache &LX, uint32_t index, int prev) {
   for (int i = 0; i < LX.ASSOC; i++) {
      if (LX.sets[index][i].LRU < prev) LX.sets[index][i].LRU++;
      else if (LX.sets[index][i].LRU == prev) LX.sets[index][i].LRU = 0;
   }
}

uint32_t find_MRU(Cache LX, uint32_t index) {
   uint32_t max = 0;
   uint32_t element = 0;
   for (int i = 0; i < LX.ASSOC; i++) {
      if (LX.sets[index][i].LRU > max) {
         max = LX.sets[index][i].LRU;
         element = i;
      }
   }
   return element;
}

void eviction(Cache LX, uint32_t address) {
   
}

// write back write allocate
uint32_t write_command(Cache &LX, uint32_t address, char read_write) {
   uint32_t index = (address >> LX.nums_block_offset) & ((1<< LX.nums_index) - 1);
   uint32_t tag = address >> (LX.nums_index + LX.nums_block_offset);
   int prev_lru = 0;
   //uint32_t MRU

   for (int i = 0; i < LX.ASSOC; i++) {
      if (LX.sets[index][i].value == address) {
         prev_lru = i;
         update_lru(LX, index, prev_lru);
         LX.sets[index][i].valid = true;
         LX.sets[index][i].dirty = true;
         LX.sets[index][i].value = address;
         return LX.sets[index][i].value;
      }
   }

   if (LX.next_cache != NULL) {
      write_command(*LX.next_cache, address, read_write);
   }
   else {
      uint32_t MRU = find_MRU(LX, index);
      uint32_t MRU_address = LX.sets[index][MRU].value;
      LX.sets[index][MRU].dirty = true;
      LX.sets[index][MRU].valid = true;
      LX.sets[index][MRU].value = address;
      LX.sets[index][MRU].LRU = 0;
      return MRU_address;
   }

   

   // switch (read_write) {
   //    case READ_COM:
   //    read_write = READ_MISS;
   //    return false;
   //    break;
   //    case WRITE_COM:
   //    read_write = WRITE_MISS;
   //    return false;
   //    break;
   //    case WRITE_HIT:
   //    // L1
   //    for (int i = 0; i < LX.ASSOC; i++) {
   //       if (LX.sets[index][i].value == tag) {
   //          Mem_Space addr = Mem_Space(false, true, tag, 0);
   //          Mem_Space temp = LX.sets[index][0];
   //          LX.sets[index][0] = addr;
   //          temp.LRU = i;
   //          LX.sets[index][i] = temp;
   //          return true;
   //       }
   //    }
   //    break;
   //    case READ_HIT:
   //    // do nothing if hit in L1
   //    return true;
   //    break;
   //    default:
   //    break;
   // }

   // switch (read_write) {
   //    case READ_COM:
   //    break;
   //    case WRITE_COM:
   //    // for (int i = 0; i < LX.ASSOC; i++) {
   //    //    if (LX.sets[index][i].valid != false) {
   //    //       read_write = WRITE_COM;
   //    //       break;
   //    //    }
   //    //    read_write = EMPTY_SET;
   //    // }
   //    for (int i = 0; i < LX.ASSOC; i++) {
   //       if (LX.sets[index][i].value == tag) {
   //          Mem_Space addr = Mem_Space(false, true, tag, 0);
   //          Mem_Space temp = LX.sets[index][0];
   //          LX.sets[index][0] = addr;
   //          temp.LRU = i;
   //          LX.sets[index][i] = temp;
   //       }
   //    }
   //    break;
   //    default:
   //    break;
   // }
   return 1;
}

uint32_t read_command(Cache &LX, uint32_t address) {
   uint32_t index = (address >> LX.nums_block_offset) & ((1<< LX.nums_index) - 1);
   uint32_t tag = address >> (LX.nums_index + LX.nums_block_offset);
   int prev_lru = 0;

   for (int i = 0; i < LX.ASSOC; i++) {
      if (LX.sets[index][i].value == address) {
         prev_lru = i;
         update_lru(LX, index, prev_lru);
         if (LX.next_cache == NULL) {
            L2_read++;
            L1_read_miss++;
            LX.sets[index][i].dirty = false;
         }
         else {
            L1_read++;
         }
         return address;
      }
   }

   if (LX.next_cache != NULL) {
      read_command(*LX.next_cache, address);

   }
   else {
      L2_read_miss++;
      uint32_t MRU = find_MRU(LX, index);
      
   }
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

   Cache L1 = Cache(params.BLOCKSIZE, params.L1_SIZE, params.L1_ASSOC);
   printf("%d\n%d\n%d\n",L1.BLOCKSIZE, L1.SIZE, L1.ASSOC);

   if (params.L2_ASSOC > 0 && params.L2_SIZE > 0) {
      Cache L2 = Cache(params.BLOCKSIZE, params.L2_SIZE, params.L2_ASSOC);
      printf("%d\n%d\n%d\n",L2.BLOCKSIZE, L2.SIZE, L2.ASSOC);
      L1.next_cache = &L2;
   }

   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw == 'r') {
         printf("r %x\n", addr);
         read_command(L1,addr);
      }
         
      else if (rw == 'w')
         printf("w %x\n", addr);
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }

      ///////////////////////////////////////////////////////
      // Issue the request to the L1 cache instance here.
      ///////////////////////////////////////////////////////
      
    }

    printf("L1 read: %d\n", L1_read);
    printf("L1 read miss: %d\n", L1_read_miss);
    printf("L2 read: %d\n", L2_read);
    printf("L2 read miss: %d\n", L2_read_miss);
    return(0);
}
