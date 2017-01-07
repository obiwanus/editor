
#if BUILD_INTERNAL
{
  Area *main_area = state->UI->areas[0];

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
  draw_string(main_area, 10, 10, fps_string, 0x00FFFFFF);

#if 1  // Display performance counters
  int line_start = 50;
  int line_height = 25;
  char perf_counters[200];
  for (int i = 0; i < g_num_perf_counters; ++i) {
    ED_Perf_Counter *counter = g_performance_counters + i;
    sprintf(perf_counters, "%s:%d (%s): %'u | %'lu", counter->file,
            counter->line, counter->function, counter->hits, counter->ticks);
    draw_string(main_area, 10, line_start, perf_counters, 0x00FFFFFF);
    line_start += line_height;

    // Reset the hits number
    counter->hits = 0;
  }
#endif
}
#endif
