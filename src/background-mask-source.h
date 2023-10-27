#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *background_mask_source_getname(void *unused);
void *background_mask_source_create(obs_data_t *settings, obs_source_t *source);
void background_mask_source_destroy(void *data);
void background_mask_source_defaults(obs_data_t *settings);
obs_properties_t *background_mask_source_properties(void *data);
void background_mask_source_update(void *data, obs_data_t *settings);
void background_mask_source_activate(void *data);
void background_mask_source_deactivate(void *data);
void background_mask_source_video_tick(void *data, float seconds);
void background_mask_source_video_render(void *data, gs_effect_t *_effect);
uint32_t background_mask_source_get_width(void *data);
uint32_t background_mask_source_get_height(void *data);

#ifdef __cplusplus
}
#endif
