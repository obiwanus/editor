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
#define TIME_END(name, count) timed_block##name.end_timed_block(count, true)

#else  // not BUILD_INTERNAL

#define TIMED_BLOCK()

#endif

struct ED_Perf_Counter {
  char *file;
  char *function;
  int line;
  u32 hits;
  u64 ticks;
  u64 min;
  u64 max;
};

struct ED_FPS_Counter {
  int value;
  int min;
  int max;
  int frame_count;
};

global ED_FPS_Counter g_FPS;
extern int g_num_perf_counters;
extern ED_Perf_Counter g_performance_counters[];

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
  }

  void end_timed_block(int count, bool destroy = false) {
    perf_counter->ticks += __rdtsc() - this->last_timestamp;
    perf_counter->hits += count;
    if (destroy) {
      destroyed = true;
    }
  }

  ~Timed_Block() {
    if (destroyed) return;
    end_timed_block(1);
  }
};

#endif  // __ED_DEBUG_H__
