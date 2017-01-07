#ifndef __ED_DEBUG_H__
#define __ED_DEBUG_H__

#if BUILD_INTERNAL

#define TIMED_BLOCK__(counter)                                              \
  Timed_Block timed_block##counter((char *)__FILE__, (char *) __FUNCTION__, \
                                   __LINE__, counter);
#define TIMED_BLOCK_(counter) TIMED_BLOCK__(counter)
#define TIMED_BLOCK() TIMED_BLOCK_(__COUNTER__)

#define TIME_BEGIN(name) \
  Timed_Block timed_block##name((char *)__FILE__, #name, __LINE__, __COUNTER__);
#define TIME_END(name) timed_block##name.end_timed_block(true)

#else  // not BUILD_INTERNAL

#define TIMED_BLOCK()

#endif

struct Timed_Block {
  ED_Perf_Counter *perf_counter;
  u64 last_timestamp;
  bool destroyed = false;

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

  void end_timed_block(bool destroy = false) {
    perf_counter->ticks += __rdtsc() - this->last_timestamp;
    // printf("%s: %lu\n", perf_counter->function, perf_counter->ticks);
    perf_counter->hits++;
    if (destroy) {
      destroyed = true;
    }
  }

  ~Timed_Block() {
    if (destroyed) return;
    end_timed_block();
  }
};

#endif  // __ED_DEBUG_H__
