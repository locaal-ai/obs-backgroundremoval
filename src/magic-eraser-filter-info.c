#include "magic-eraser-filter.h"

struct obs_source_info magic_eraser_filter_info = {
	.id = "magiceraser",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = magic_eraser_filter_getname,
	.create = magic_eraser_filter_create,
	.destroy = magic_eraser_filter_destroy,
	.get_defaults = magic_eraser_filter_defaults,
	.get_properties = magic_eraser_filter_properties,
	.update = magic_eraser_filter_update,
	.activate = magic_eraser_filter_activate,
	.deactivate = magic_eraser_filter_deactivate,
	.video_tick = magic_eraser_filter_video_tick,
	.video_render = magic_eraser_filter_video_render,
};
