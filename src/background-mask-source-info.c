#include "background-mask-source.h"

struct obs_source_info background_removal_mask_source_info = {
	.id = "background_removal_mask_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = background_mask_source_getname,
	.create = background_mask_source_create,
	.destroy = background_mask_source_destroy,
	.get_defaults = background_mask_source_defaults,
	.get_properties = background_mask_source_properties,
	.update = background_mask_source_update,
	.activate = background_mask_source_activate,
	.deactivate = background_mask_source_deactivate,
	.video_tick = background_mask_source_video_tick,
	.video_render = background_mask_source_video_render,
    .get_width = background_mask_source_get_width,
    .get_height = background_mask_source_get_height,
};
