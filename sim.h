#ifndef SIM_CACHE_H
#define SIM_CACHE_H

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

   Mem_Space() {
      dirty = false;
      valid = false;
   }

   Mem_Space(bool dbit, bool vbit) {
      this->dirty = dbit;
      this->valid = vbit;
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

   // Constructor
   Cache() {
      BLOCKSIZE = 0;
      SIZE = 0;
      ASSOC = 0;
      nums_sets = 0;
      nums_index = 0;
   }
   
   Cache(uint32_t inputBlocksize, uint32_t inputSize, uint32_t inputAssoc) {
      this->BLOCKSIZE = inputBlocksize;
      this->SIZE = inputSize;
      this->ASSOC = inputAssoc;
      this->nums_sets = (this->SIZE)/(this->ASSOC * this->BLOCKSIZE);
      this->nums_index = log2(nums_sets);
      this->nums_block_offset = log2(BLOCKSIZE);
      this->nums_tag = ADDRESS_SIZE - nums_index - nums_block_offset;
   }
   

   // Destructor
   //Cache::~Cache();
};

class Set {
   public:
   vector<vector<Mem_Space>> sets;

   Set(Cache LX) {
      
   }

};

// Class Set {
//    public:
//    Set(Cache cachex) {
//       set
//    }

// };
#endif
