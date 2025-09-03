#ifndef SIM_CACHE_H
#define SIM_CACHE_H

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
class Cache {
   public:
   cache_params_t parameters;

   // Constructor
   Cache(cache_params_t param) {
      parameters.BLOCKSIZE = param.BLOCKSIZE;
      parameters.L1_SIZE = param.L1_SIZE;
      parameters.L1_ASSOC = param.L1_ASSOC;
      parameters.L2_SIZE = param.L2_SIZE;
      parameters.L2_ASSOC = param.L2_ASSOC;
      parameters.PREF_N = param.PREF_N;
      parameters.PREF_M = param.PREF_M;
   }

   // Destructor
   ~Cache();
}
#endif
