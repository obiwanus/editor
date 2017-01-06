#ifndef __ED_DEBUG_H__
#define __ED_DEBUG_H__

#if BUILD_INTERNAL
#define TIMED_BLOCK()                                                        \
  Timed_Block timed_block##__LINE__((char *)__FILE__, (char *) __FUNCTION__, \
                                    __LINE__, __COUNTER__);
#else
#define TIMED_BLOCK()
#endif

struct Timed_Block {
  ED_Perf_Counter *perf_counter;

  Timed_Block(char *file, char *function, int line, int counter) {
    perf_counter = g_performance_counters + counter;
    perf_counter->file = file;
    perf_counter->function = function;
    perf_counter->line = line;
    perf_counter->ticks = __rdtsc();
  }

  ~Timed_Block() {
    perf_counter->hits++;
    perf_counter->ticks = __rdtsc() - perf_counter->ticks;
    // printf("%s: %lu\n", perf_counter->function, perf_counter->ticks);
  }
};

#endif  // __ED_DEBUG_H__
