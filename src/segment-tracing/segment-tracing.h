#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
  SEGMENT_TYPE_PLUGIN_LOAD = 0,
  SEGMENT_TYPE_PLUGIN_UNLOAD,
  SEGMENT_TYPE_FILTER_CREATE,
  SEGMENT_TYPE_FILTER_DESTROY,
  SEGMENT_TYPE_FILTER_UPDATE,
  SEGMENT_TYPE_FILTER_ACTIVATED,
  SEGMENT_TYPE_FILTER_DEACTIVATED,
  SEGMENT_TYPE_FILTER_USAGE_STATS,
  SEGMENT_TYPE_FILTER_ERROR,
  SEGMENT_TYPE_PLUGIN_INIT_TRACE,
  SEGMENT_TYPE_FILTER_REMOVE,
};

int send_segment_trace(int segment_type, int info);
void segment_tracing_init(void);
void segment_tracing_init_tracing(void);
void segment_tracing_deinit(void);
void segment_tracing_show_dialog(void);

#ifdef __cplusplus
}
#endif
