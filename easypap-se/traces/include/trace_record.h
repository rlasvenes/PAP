#ifndef TRACE_RECORD_IS_DEF
#define TRACE_RECORD_IS_DEF

#define DEFAULT_EZV_TRACE_DIR "traces/data"
#define DEFAULT_EZV_TRACE_BASE "ezv_trace_current"
#define DEFAULT_EZV_TRACE_EXT  ".evt"
#define DEFAULT_EZV_TRACE_FILE DEFAULT_EZV_TRACE_BASE DEFAULT_EZV_TRACE_EXT
#define DEFAULT_EASYVIEW_FILE DEFAULT_EZV_TRACE_DIR "/" DEFAULT_EZV_TRACE_FILE

extern unsigned do_trace;

void trace_record_init (char *file, unsigned cpu, unsigned dim, char *label);
void __trace_record_start_iteration (long time);
void __trace_record_end_iteration (long time);
void __trace_record_start_tile (long time, unsigned cpu);
void __trace_record_end_tile (long time, unsigned cpu, unsigned x, unsigned y,
                              unsigned w, unsigned h);
void trace_record_finalize (void);

#define trace_record_start_iteration(t)                                        \
  do {                                                                         \
    if (do_trace)                                                              \
      __trace_record_start_iteration (t);                                      \
  } while (0)

#define trace_record_end_iteration(t)                                          \
  do {                                                                         \
    if (do_trace)                                                              \
      __trace_record_end_iteration (t);                                        \
  } while (0)

#define trace_record_start_tile(t, c)                                          \
  do {                                                                         \
    if (do_trace)                                                              \
      __trace_record_start_tile ((t), (c));                                    \
  } while (0)

#define trace_record_end_tile(t, c, x, y, w, h)                                \
  do {                                                                         \
    if (do_trace)                                                              \
      __trace_record_end_tile ((t), (c), (x), (y), (w), (h));                  \
  } while (0)

#endif
