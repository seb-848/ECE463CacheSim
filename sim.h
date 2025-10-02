#ifndef SIM_CACHE_H
#define SIM_CACHE_H

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>

const uint32_t ADDRESS_SIZE = 32;

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

// Put additional data structures here as per your requirement.

// cache class
// typedef struct {
   
// }

class Mem_Space {
   public:
   bool dirty;
   bool valid;
   uint32_t value;
   uint32_t LRU;

   Mem_Space() {
      dirty = false;
      valid = false;
      value = 0;
      LRU = 0;
   }

   Mem_Space(bool dbit, bool vbit, uint32_t val, uint32_t LRU_val) {
      this->dirty = dbit;
      this->valid = vbit;
      this->value = val;
      this->LRU = LRU_val;
   }
};

//Cache
class Cache {
   public:
   // Member Variables
   uint32_t BLOCKSIZE;
   uint32_t SIZE;
   uint32_t ASSOC;
   uint32_t nums_sets;
   uint32_t nums_tag;
   uint32_t nums_index;
   uint32_t nums_block_offset;
   std::vector<std::vector<Mem_Space>> sets;
   Mem_Space default_block = Mem_Space(false, false, 0, 0);
   Cache* next_cache;

   // Constructor
   Cache() {
      BLOCKSIZE = 0;
      SIZE = 0;
      ASSOC = 0;
      nums_sets = 0;
      nums_index = 0;
      next_cache = NULL;
   }
   
   Cache(uint32_t inputBlocksize, uint32_t inputSize, uint32_t inputAssoc) {
      this->BLOCKSIZE = inputBlocksize;
      this->SIZE = inputSize;
      this->ASSOC = inputAssoc;
      this->nums_sets = (this->SIZE)/(this->ASSOC * this->BLOCKSIZE);
      this->nums_index = log2(nums_sets);
      this->nums_block_offset = log2(BLOCKSIZE);
      this->nums_tag = ADDRESS_SIZE - nums_index - nums_block_offset;
      default_block.LRU = this->ASSOC;

      sets.assign(this->nums_sets, std::vector<Mem_Space>(this->ASSOC, default_block));
   }
   

   // Destructor
   //Cache::~Cache();
};

// class Set {
//    public:
//    //std::vector<std::vector<Mem_Space>> sets(int set_num, std::vector<Mem_Space>(int ways, Mem_Space space()));
//    Mem_Space block = Mem_Space(false, false, 0, 0);
//    Set() {
//       std::vector<std::vector<Mem_Space>> sets(int set_num, std::vector<Mem_Space>(int ways, Mem_Space space()));
//    }

//    Set(Cache CacheLX) {
//       std::vector<std::vector<Mem_Space>> sets(CacheLX.nums_sets, std::vector<Mem_Space>(CacheLX.ASSOC, block));
//    }
// };


class Address {
   public:
   uint32_t tag;
   uint32_t index;
   uint32_t blockOffset;

   Address() {
      tag = 0;
      index = 0;
      blockOffset = 0;
   }

   Address(uint32_t address, Cache LX) {
      this->blockOffset = address & ((1 << LX.nums_block_offset) - 1);
      this->index = (address >> LX.nums_block_offset) & ((1<< LX.nums_index) - 1);
      this->tag = address >> (LX.nums_index + LX.nums_block_offset);
   }
};

#endif
