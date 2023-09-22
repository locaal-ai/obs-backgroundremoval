#include "magic-eraser-filter.h"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <opencv2/photo.hpp>

#include "plugin-support.h"
#include "consts.h"
#include "obs-utils/obs-utils.h"
#include "ort-utils/ort-session-utils.h"
#include "mask-editor/MaskEditorDialog.hpp"


struct magic_eraser_filter : public filter_data_base {
  cv::Mat eraserMask;
  cv::Mat outputBGRA;
};

const char *magic_eraser_filter_getname(void *unused) {
	UNUSED_PARAMETER(unused);
  return "Magic Eraser";
}

void *magic_eraser_filter_create(obs_data_t *settings, obs_source_t *source) {
	UNUSED_PARAMETER(settings);
	struct magic_eraser_filter *tf = new magic_eraser_filter();

	tf->source = source;
	tf->isDisabled = false;
	tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

	tf->eraserMask = cv::Mat();

	return tf;
}

void magic_eraser_filter_destroy(void *data) {
	struct magic_eraser_filter *tf = reinterpret_cast<magic_eraser_filter *>(data);

	delete tf;
}

void magic_eraser_filter_defaults(obs_data_t *settings) {
	UNUSED_PARAMETER(settings);

}

obs_properties_t *magic_eraser_filter_properties(void *data) {

	obs_properties_t *props = obs_properties_create();

	// add a button to open the mask editor dialog
	obs_properties_add_button2(props, "edit_mask", "Edit Mask",
				  [](obs_properties_t *props, obs_property_t *property,
				     void *data_) {
					  UNUSED_PARAMETER(props);
					  UNUSED_PARAMETER(property);
					  struct magic_eraser_filter *tf =
						  reinterpret_cast<magic_eraser_filter *>(data_);

					  // create the mask editor dialog
						MaskEditorDialog *dialog = new MaskEditorDialog((QWidget *)obs_frontend_get_main_window(), tf->eraserMask);
						dialog->setAttribute(Qt::WA_DeleteOnClose);
						dialog->show();
						return true;
				  },
				  data);

	return props;
}

void magic_eraser_filter_update(void *data, obs_data_t *settings) {
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
}

void magic_eraser_filter_activate(void *data) {
	UNUSED_PARAMETER(data);
}

void magic_eraser_filter_deactivate(void *data) {
	UNUSED_PARAMETER(data);
}

void runMagicEraser(struct magic_eraser_filter *tf, const cv::Mat &inputImageBGRA,
        cv::Mat &outputImageBGR) {
  // Perform inpainting on the input image using the mask
  cv::Mat imageBGR;
  cv::cvtColor(inputImageBGRA, imageBGR, cv::COLOR_BGRA2BGR);
  cv::inpaint(imageBGR, tf->eraserMask, outputImageBGR, 3, cv::INPAINT_TELEA);
	outputImageBGR.setTo(cv::Scalar(0, 0, 255), tf->eraserMask);
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

		if (tf->eraserMask.empty()) {
			obs_log(LOG_INFO, "magic_eraser_filter_video_tick: eraserMask is empty. Creating a new one.");
			tf->eraserMask = cv::Mat::zeros(imageBGRA.size(), CV_8UC1);
			// draw a circle in the middle of the mask
			cv::circle(tf->eraserMask, cv::Point(imageBGRA.cols / 2, imageBGRA.rows / 2), 100, cv::Scalar(255, 255, 255), -1);
		}
	}

	cv::Mat outputImage;
	try {
		runMagicEraser(tf, imageBGRA, outputImage);
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

void magic_eraser_filter_video_render(void *data, gs_effect_t *do_not_use) {
	UNUSED_PARAMETER(do_not_use);

	struct magic_eraser_filter *tf = reinterpret_cast<magic_eraser_filter *>(data);

	// Get input from source
	uint32_t width, height;
	if (!getRGBAFromStageSurface(tf, width, height)) {
		obs_log(LOG_ERROR, "Failed to get RGBA from stage surface");
		obs_source_skip_video_filter(tf->source);
		return;
	}

	// Engage filter
	if (!obs_source_process_filter_begin(tf->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING)) {
		obs_log(LOG_ERROR, "Failed to begin filter");
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

	// get default effect
	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	// Set effect parameters
	gs_eparam_t *effectImage = gs_effect_get_param_by_name(effect, "image");
	if (!effectImage) {
		blog(LOG_ERROR, "Failed to get effect parameter");
		obs_source_skip_video_filter(tf->source);
		return;
	}
	gs_effect_set_texture(effectImage, outputTexture);

	// Render texture
	gs_blend_state_push();
	gs_reset_blend_state();

	obs_source_process_filter_end(tf->source, effect, tf->outputBGRA.cols, tf->outputBGRA.rows);

	gs_blend_state_pop();

	gs_texture_destroy(outputTexture);
}
