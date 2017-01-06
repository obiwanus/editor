
#if BUILD_INTERNAL
{
  Pixel_Buffer *buffer = &g_pixel_buffer;

  // Display FPS
  if (g_FPS.x < g_FPS.y) {
    g_FPS.y = g_FPS.x;  // min
  }
  if (g_FPS.x > g_FPS.z) {
    g_FPS.z = g_FPS.x;  // max
  }
  char fps_string[100];
  sprintf(fps_string, "FPS: %d, min: %d, max: %d", g_FPS.x, g_FPS.y, g_FPS.z);
  // char *fps_string = "60";
  draw_string(buffer, 10, 10, fps_string, 0x00FFFFFF);

#if 1  // Display performance counters
  char perf_counters[1000];
  char *pc_string = perf_counters;
  for (int i = 0; i < g_num_perf_counters; ++i) {
    ED_Perf_Counter *counter = g_performance_counters + i;
    int chars_written = sprintf(pc_string, "%s:%d (%s): %u | %lu\n",
                                counter->file, counter->line, counter->function,
                                counter->hits, counter->ticks);
    if (chars_written > 0) {
      pc_string += chars_written;
    }

    // Reset the hits number
    counter->hits = 0;
  }
  draw_string(buffer, 10, 100, perf_counters, 0x00FFFFFF);
#endif
}
#endif
