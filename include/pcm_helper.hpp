#ifndef _PCM_HELPER_HPP_
#define _PCM_HELPER_HPP_

#include "common.hpp"

#ifdef ENABLE_PCM
#include <cpucounters.h>
#include <iostream>

namespace pcm_helper {
   typedef SystemCounterState system_counter_state_t;
   PCM* pcm_m = NULL;

   int32_t pcm_init() {
      pcm_m = PCM::getInstance();
      int32_t ret_val = 0;

      switch(pcm_m->program()) {
         case PCM::Success:
            cout << "PCM initialized" << endl;
            break;
         case PCM::PMUBusy:
            pcm_m->resetPMU();
            ret_val = -1;
            break;
         default:
            ret_val = -1;
            break;
      }

      return ret_val;
   }

   int32_t pcm_cleanup() {
      pcm_m->resetPMU();
      return 0;
   }

   system_counter_state_t pcm_get_counter_state() { return getSystemCounterState(); }

   void pcm_print_counter_stats(system_counter_state_t before_sstate, system_counter_state_t after_sstate) {
      std::cout
         << "Instructions per clock: " << getIPC(before_sstate, after_sstate) << std::endl
         << "L2 cache hit ratio: " << getL2CacheHitRatio(before_sstate, after_sstate) << std::endl
         << "L3 cache hit ratio: " << getL3CacheHitRatio(before_sstate, after_sstate) << std::endl
         << "L2 cache misses: " << getL2CacheMisses(before_sstate, after_sstate) << std::endl
         << "L3 cache misses: " << getL3CacheMisses(before_sstate, after_sstate) << std::endl
         << "Cycles lost due to L2 cache misses: " << getCyclesLostDueL2CacheMisses(before_sstate, after_sstate) << std::endl
         << "Cycles lost due to L3 cache misses: " << getCyclesLostDueL3CacheMisses(before_sstate, after_sstate) << std::endl
         << "Bytes read: " << getBytesReadFromMC(before_sstate, after_sstate) << std::endl;
   }
}
#else
namespace pcm_helper {
   // Stubs if PCM is not included
   typedef int32_t system_counter_state_t;
   int32_t pcm_init() { return 0; }
   int32_t pcm_cleanup() { return 0; }
   system_counter_state_t pcm_get_counter_state() { return 0; }
   void pcm_print_counter_stats(system_counter_state_t before_sstate, system_counter_state_t after_sstate) {}
}
#endif

#endif
