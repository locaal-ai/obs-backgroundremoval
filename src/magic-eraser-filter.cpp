#include <obs-module.h>

#include <opencv2/photo.hpp>

#include "plugin-support.h"
#include "consts.h"
#include "obs-utils/obs-utils.h"
#include "ort-utils/ort-session-utils.h"


struct magic_eraser_filter : public filter_data_base {
  cv::Mat eraserMask;
  cv::Mat outputBGRA;
};

const char *magic_eraser_filter_getname(void *unused) {
  return "Magic Eraser";
}

void *magic_eraser_filter_create(obs_data_t *settings, obs_source_t *source) {

}
void magic_eraser_filter_destroy(void *data) {

}

void magic_eraser_filter_defaults(obs_data_t *settings) {

}

obs_properties_t *magic_eraser_filter_properties(void *data) {

}

void magic_eraser_filter_update(void *data, obs_data_t *settings) {

}

void magic_eraser_filter_activate(void *data) {

}

void magic_eraser_filter_deactivate(void *data) {

}

void runMagicEraser(struct magic_eraser_filter *tf, cv::Mat &inputImageBGRA,
        cv::Mat &outputImageBGR) {
  // Perform inpainting on the input image using the mask
  cv::Mat mask;
  cv::cvtColor(tf->eraserMask, mask, cv::COLOR_BGRA2GRAY);
  cv::inpaint(inputImageBGRA, mask, outputImageBGR, 3, cv::INPAINT_TELEA);
}

void magic_eraser_filter_video_tick(void *data, float seconds) {
  UNUSED_PARAMETER(seconds);
	struct magic_eraser_filter *tf = reinterpret_cast<magic_eraser_filter *>(data);

	if (tf->isDisabled) {
		return;
	}

	if (!obs_source_enabled(tf->source)) {
		return;
	}

	if (tf->inputBGRA.empty()) {
		return;
	}

	// Get input image from source rendering pipeline
	cv::Mat imageBGRA;
	{
		std::unique_lock<std::mutex> lock(tf->inputBGRALock,
						  std::try_to_lock);
		if (!lock.owns_lock()) {
			return;
		}
		imageBGRA = tf->inputBGRA.clone();
	}

	cv::Mat outputImage;
	try {
		if (!runMagicEraser(tf, imageBGRA, outputImage)) {
			return;
		}
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "Exception caught: %s", e.what());
		return;
	}

	// Put output image back to source rendering pipeline
	{
		std::unique_lock<std::mutex> lock(tf->outputLock,
						  std::try_to_lock);
		if (!lock.owns_lock()) {
			return;
		}

		// convert to RGBA
		cv::cvtColor(outputImage, tf->outputBGRA, cv::COLOR_BGR2RGBA);
	}
}

void magic_eraser_filter_video_render(void *data, gs_effect_t *_effect) {
  UNUSED_PARAMETER(_effect);

	struct magic_eraser_filter *tf = reinterpret_cast<magic_eraser_filter *>(data);

	// Get input from source
	uint32_t width, height;
	if (!getRGBAFromStageSurface(tf, width, height)) {
		obs_source_skip_video_filter(tf->source);
		return;
	}

	// Engage filter
	if (!obs_source_process_filter_begin(tf->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING)) {
		obs_source_skip_video_filter(tf->source);
		return;
	}

	// Put output into texture
	gs_texture_t *outputTexture = nullptr;
	{
		std::lock_guard<std::mutex> lock(tf->outputLock);
		outputTexture = gs_texture_create(
			tf->outputBGRA.cols, tf->outputBGRA.rows, GS_BGRA, 1,
			(const uint8_t **)&tf->outputBGRA.data, 0);
		if (!outputTexture) {
			blog(LOG_ERROR, "Failed to create output texture");
			obs_source_skip_video_filter(tf->source);
			return;
		}
	}

	gs_eparam_t *blendimage =
		gs_effect_get_param_by_name(_effect, "image");
	gs_effect_set_texture(blendimage, outputTexture);
	// Render texture
	gs_blend_state_push();
	gs_reset_blend_state();

	obs_source_process_filter_tech_end(tf->source, _effect, 0, 0,
					   "Draw");

	gs_blend_state_pop();

	gs_texture_destroy(outputTexture);
}
