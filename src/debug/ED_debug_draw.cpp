
#if BUILD_INTERNAL
{
  Area *main_area = state->UI->areas[0];

  // Display FPS
  if (g_FPS.value < g_FPS.min) g_FPS.min = g_FPS.value;
  if (g_FPS.value > g_FPS.max) g_FPS.max = g_FPS.value;

  char fps_string[100];
  sprintf(fps_string, "FPS: %d, min: %d, max: %d", g_FPS.value, g_FPS.min,
          g_FPS.max);
  // char *fps_string = "60";
  draw_string(main_area, 10, 10, fps_string, 0x00FFFFFF);

  g_FPS.frame_count--;

#if 1  // Display performance counters
  int line_start = 50;
  int line_height = 25;
  char perf_counters[200];
  for (int i = 0; i < g_num_perf_counters; ++i) {
    ED_Perf_Counter *counter = g_performance_counters + i;
    if (counter->hits == 0) continue;
    u64 ticks = counter->ticks / counter->hits;
    // Update min max
    if (ticks < counter->min) counter->min = ticks;
    if (ticks > counter->max) counter->max = ticks;
    sprintf(perf_counters, "%s:%d (%s): %'u | min: %'lu, max: %'lu | %'lu",
            counter->file, counter->line, counter->function, counter->hits,
            counter->min, counter->max, ticks);
    draw_string(main_area, 10, line_start, perf_counters, 0x00FFFFFF);
    line_start += line_height;

    // Reset the hits number
    counter->hits = 0;
    counter->ticks = 0;

    // Align min and max with FPS
    if (g_FPS.frame_count < 0) {
      counter->min = (u64)1 << 63;
      counter->max = 0;
    }
  }
#endif

  // Reset FPS min and max values
  if (g_FPS.frame_count < 0) {
    g_FPS.frame_count = 60;
    g_FPS.min = 1000;
    g_FPS.max = 0;
  }
}
#endif
