#ifndef __ED_DEBUG_H__
#define __ED_DEBUG_H__

#if BUILD_INTERNAL
#define TIMED_BLOCK__(counter)                                                        \
  Timed_Block timed_block##counter((char *)__FILE__, (char *) __FUNCTION__, \
                                    __LINE__, counter);
#define TIMED_BLOCK_(counter) TIMED_BLOCK__(counter)
#define TIMED_BLOCK() TIMED_BLOCK_(__COUNTER__)
#else
#define TIMED_BLOCK()
#endif

struct Timed_Block {
  ED_Perf_Counter *perf_counter;
  u64 last_timestamp;

  Timed_Block(char *file, char *function, int line, int counter) {
    perf_counter = g_performance_counters + counter;
    perf_counter->file = file;
    perf_counter->function = function;
    perf_counter->line = line;
    this->last_timestamp = __rdtsc();
    if (!perf_counter->hits) {
      perf_counter->ticks = 0;
    }
  }

  ~Timed_Block() {
    perf_counter->ticks += __rdtsc() - this->last_timestamp;
    // printf("%s: %lu\n", perf_counter->function, perf_counter->ticks);
    perf_counter->hits++;
  }
};

#endif  // __ED_DEBUG_H__
