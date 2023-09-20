#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *magic_eraser_filter_getname(void *unused);
void *magic_eraser_filter_create(obs_data_t *settings, obs_source_t *source);
void magic_eraser_filter_destroy(void *data);
void magic_eraser_filter_defaults(obs_data_t *settings);
obs_properties_t *magic_eraser_filter_properties(void *data);
void magic_eraser_filter_update(void *data, obs_data_t *settings);
void magic_eraser_filter_activate(void *data);
void magic_eraser_filter_deactivate(void *data);
void magic_eraser_filter_video_tick(void *data, float seconds);
void magic_eraser_filter_video_render(void *data, gs_effect_t *_effect);

#ifdef __cplusplus
}
#endif
