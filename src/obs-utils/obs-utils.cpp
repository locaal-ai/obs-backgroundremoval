#include "obs-utils.h"

#include <obs-module.h>

/**
  * @brief Get RGBA from the stage surface
  *
  * @param tf  The filter data
  * @param width  The width of the stage surface (output)
  * @param height  The height of the stage surface (output)
  * @return true  if successful
  * @return false if unsuccessful
*/
bool getRGBAFromStageSurface(filter_data *tf, uint32_t &width, uint32_t &height)
{

	if (!obs_source_enabled(tf->source)) {
		obs_log(LOG_ERROR, "Source is not enabled");
		return false;
	}

	obs_source_t *target = tf->source;

	// if source is a filter, get the target of the filter
	if (obs_source_get_type(tf->source) == OBS_SOURCE_TYPE_FILTER) {
		target = obs_filter_get_target(tf->source);
		if (!target) {
			obs_log(LOG_ERROR, "Could not get target for filter");
			return false;
		}
	}

	width = obs_source_get_base_width(target);
	height = obs_source_get_base_height(target);
	if (width == 0 || height == 0) {
		obs_log(LOG_ERROR, "Width or height is 0");
		return false;
	}
	gs_texrender_reset(tf->texrender);
	if (!gs_texrender_begin(tf->texrender, width, height)) {
		obs_log(LOG_ERROR, "Could not begin texrender");
		return false;
	}
	struct vec4 background;
	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f,
		 100.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(tf->texrender);

	if (tf->stagesurface) {
		uint32_t stagesurf_width = gs_stagesurface_get_width(tf->stagesurface);
		uint32_t stagesurf_height = gs_stagesurface_get_height(tf->stagesurface);
		if (stagesurf_width != width || stagesurf_height != height) {
			gs_stagesurface_destroy(tf->stagesurface);
			tf->stagesurface = nullptr;
		}
	}
	if (!tf->stagesurface) {
		tf->stagesurface = gs_stagesurface_create(width, height, GS_BGRA);
	}
	gs_stage_texture(tf->stagesurface, gs_texrender_get_texture(tf->texrender));
	uint8_t *video_data;
	uint32_t linesize;
	if (!gs_stagesurface_map(tf->stagesurface, &video_data, &linesize)) {
		obs_log(LOG_ERROR, "Cannot map stage surface");
		return false;
	}
    obs_log(LOG_INFO, "linesize: %d, width: %d, height: %d", linesize, width, height);
    if (linesize != width * 4) {
        obs_log(LOG_WARNING, "linesize %d != width %d * 4", linesize, width);
    }
	{
		std::lock_guard<std::mutex> lock(tf->inputBGRALock);
		if (tf->inputBGRA.cols != width || tf->inputBGRA.rows != height || tf->inputBGRA.step != linesize) {
            obs_log(LOG_INFO, "Reallocating inputBGRA");
			tf->inputBGRA = cv::Mat(height, width, CV_8UC4, video_data, linesize);
		} else {
            obs_log(LOG_INFO, "Copying inputBGRA");
			memcpy(tf->inputBGRA.data, video_data, width * 3 * height);
		}
	}
	gs_stagesurface_unmap(tf->stagesurface);
	return true;
}
