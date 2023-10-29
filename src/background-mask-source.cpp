#include "background-mask-source.h"
#include "FilterData.h"
#include "plugin-support.h"
#include "obs-utils/obs-utils.h"

struct background_mask_source_data : public filter_data {
	char *background_source;
	uint32_t width;
	uint32_t height;
};

bool add_sources_to_list(void *list_property, obs_source_t *source)
{
	// Only add input source_video sources
	if (!(obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) ||
	    obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT) {
		return true;
	}

	obs_property_t *sources = (obs_property_t *)list_property;
	const char *name = obs_source_get_name(source);
	obs_property_list_add_string(sources, name, name);
	return true;
}

const char *background_mask_source_getname(void *unused)
{
	return "Background Removal Mask Source";
}

void *background_mask_source_create(obs_data_t *settings, obs_source_t *source)
{
	background_mask_source_data *bgms_data = new background_mask_source_data();
	bgms_data->background_source = nullptr;
    bgms_data->source = nullptr;
	bgms_data->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

	background_mask_source_update(bgms_data, settings);
	return (void *)bgms_data;
}

void background_mask_source_destroy(void *data)
{
	background_mask_source_data *bgms_data = (background_mask_source_data *)data;

	if (bgms_data) {
		obs_enter_graphics();
		gs_texrender_destroy(bgms_data->texrender);
		if (bgms_data->stagesurface) {
			gs_stagesurface_destroy(bgms_data->stagesurface);
		}
		obs_leave_graphics();
		bgms_data->~background_mask_source_data();
		delete bgms_data;
		bgms_data = nullptr;
	}
}

void background_mask_source_defaults(obs_data_t *settings) {}

obs_properties_t *background_mask_source_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// Add sources for background removal
	obs_property_t *sources =
		obs_properties_add_list(props, "background_source", "Source for background",
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_enum_sources(add_sources_to_list, sources);

	return props;
}

void background_mask_source_update(void *data, obs_data_t *settings)
{
	background_mask_source_data *bgms_data = (background_mask_source_data *)data;

    const char* new_bg_source = obs_data_get_string(settings, "background_source");
    if (new_bg_source == nullptr || strlen(new_bg_source) == 0) {
        obs_log(LOG_ERROR, "Background source is null");
        return;
    }
    if (bgms_data->background_source != nullptr && strcmp(bgms_data->background_source, new_bg_source) == 0) {
        obs_log(LOG_INFO, "Background source is the same");
        return;
    }
    if (bgms_data->background_source != nullptr) {
        bfree(bgms_data->background_source);
    }
    if (bgms_data->source != nullptr) {
        obs_source_release(bgms_data->source);
    }

	bgms_data->background_source = bstrdup(new_bg_source);
	bgms_data->source = obs_get_source_by_name(bgms_data->background_source);
	if (bgms_data->source == nullptr) {
		obs_log(LOG_ERROR, "Background source %s not found", bgms_data->background_source);
	}
}

void background_mask_source_activate(void *data) {}

void background_mask_source_deactivate(void *data) {}

void background_mask_source_video_tick(void *data, float seconds) {}

void background_mask_source_video_render(void *data, gs_effect_t *_effect)
{
	background_mask_source_data *bgms = (background_mask_source_data *)data;

	if (bgms->source == nullptr) {
		obs_log(LOG_ERROR, "Background source not found");
		return;
	}

	uint32_t width, height;
	if (!getRGBAFromStageSurface(bgms, width, height)) {
		obs_log(LOG_ERROR, "Could not get RGBA from stage surface");
		return;
	}
	bgms->width = width;
	bgms->height = height;

	gs_texture_t *alphaTexture = nullptr;
	{
		std::lock_guard<std::mutex> lock(bgms->inputBGRALock);
		alphaTexture = gs_texture_create(bgms->inputBGRA.cols, bgms->inputBGRA.rows,
						 GS_BGRA, 1,
						 (const uint8_t **)&bgms->inputBGRA.data, 0);
		if (!alphaTexture) {
			obs_log(LOG_ERROR, "Failed to create alpha texture");
			return;
		}
	}

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	if (!effect) {
		obs_log(LOG_ERROR, "Could not get base effect");
		return;
	}

	while (gs_effect_loop(effect, "Draw"))
		obs_source_draw(alphaTexture, 0, 0, 0, 0, 0);

	gs_texture_destroy(alphaTexture);
}

uint32_t background_mask_source_get_width(void *data)
{
	background_mask_source_data *bgms = (background_mask_source_data *)data;
	if (bgms->source == nullptr) {
		obs_log(LOG_ERROR, "Background source not found");
		return 0;
	}
	return bgms->width;
}

uint32_t background_mask_source_get_height(void *data)
{
	background_mask_source_data *bgms = (background_mask_source_data *)data;
	if (bgms->source == nullptr) {
		obs_log(LOG_ERROR, "Background source not found");
		return 0;
	}
	return bgms->height;
}
