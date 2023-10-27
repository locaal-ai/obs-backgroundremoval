#include "background-mask-source.h"
#include "FilterData.h"
#include "plugin-support.h"
#include "obs-utils/obs-utils.h"

struct background_mask_source_data : public filter_data {
    char* background_source;
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

const char *background_mask_source_getname(void *unused) {
    return "Background Removal Mask Source";
}

void *background_mask_source_create(obs_data_t *settings, obs_source_t *source) {
    background_mask_source_data *bgms_data = new background_mask_source_data();
    bgms_data->background_source = nullptr;
    background_mask_source_update(bgms_data, settings);
    return (void*)bgms_data;
}

void background_mask_source_destroy(void *data) {
}

void background_mask_source_defaults(obs_data_t *settings) {
}

obs_properties_t *background_mask_source_properties(void *data) {
    obs_properties_t *props = obs_properties_create();

    // Add sources for background removal
    obs_property_t *sources = obs_properties_add_list(props, "background_source", "Source for background",
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_enum_sources(add_sources_to_list, sources);

    return props;
}

void background_mask_source_update(void *data, obs_data_t *settings) {
    background_mask_source_data *bgms_data = (background_mask_source_data*)data;
    if (bgms_data->background_source != nullptr)
        bfree(bgms_data->background_source);
    bgms_data->background_source = bstrdup(obs_data_get_string(settings, "background_source"));
    // Get the background source
    bgms_data->source = obs_get_source_by_name(bgms_data->background_source);
    if (bgms_data->source == nullptr) {
        obs_log(LOG_ERROR, "Background source %s not found", bgms_data->background_source);
    }
}

void background_mask_source_activate(void *data) {
}

void background_mask_source_deactivate(void *data) {
}

void background_mask_source_video_tick(void *data, float seconds) {
}

void background_mask_source_video_render(void *data, gs_effect_t *_effect) {
    background_mask_source_data *bgms_data = (background_mask_source_data*)data;

    if (bgms_data->source == nullptr) {
        return;
    }

    uint32_t width, height;
	if (!getRGBAFromStageSurface(bgms_data, width, height)) {
		return;
	}
}

uint32_t background_mask_source_get_width(void *data) {
    return 0;
}

uint32_t background_mask_source_get_height(void *data) {
    return 0;
}

