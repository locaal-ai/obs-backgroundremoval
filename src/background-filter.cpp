#include "background-filter.h"

#include <onnxruntime_cxx_api.h>

#ifdef _WIN32
#include <wchar.h>
#endif // _WIN32

#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>
#include <new>
#include <mutex>

#include "plugin-support.h"
#include "models/ModelSINET.h"
#include "models/ModelMediapipe.h"
#include "models/ModelSelfie.h"
#include "models/ModelRVM.h"
#include "models/ModelPPHumanSeg.h"
#include "models/ModelTCMonoDepth.h"
#include "FilterData.h"
#include "ort-utils/ort-session-utils.h"
#include "obs-utils/obs-utils.h"
#include "consts.h"
#include "segment-tracing/segment-tracing.h"

struct background_removal_filter : public filter_data {
	bool enableThreshold = true;
	float threshold = 0.5f;
	cv::Scalar backgroundColor{0, 0, 0, 0};
	float contourFilter = 0.05f;
	float smoothContour = 0.5f;
	float feather = 0.0f;

	cv::Mat backgroundMask;
	int maskEveryXFrames = 1;
	int maskEveryXFramesCount = 0;
	int64_t blurBackground = 0;
	float blurFocusPoint = 0.1f;

	gs_effect_t *effect;
	gs_effect_t *kawaseBlurEffect;
};

const char *background_filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("BackgroundRemoval");
}

/**                   PROPERTIES                     */

static bool enable_threshold_modified(obs_properties_t *ppts, obs_property_t *p,
				      obs_data_t *settings)
{
	const bool enabled = obs_data_get_bool(settings, "enable_threshold");
	p = obs_properties_get(ppts, "threshold");
	obs_property_set_visible(p, enabled);
	p = obs_properties_get(ppts, "contour_filter");
	obs_property_set_visible(p, enabled);
	p = obs_properties_get(ppts, "smooth_contour");
	obs_property_set_visible(p, enabled);
	p = obs_properties_get(ppts, "feather");
	obs_property_set_visible(p, enabled);

	return true;
}

obs_properties_t *background_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	/* Threshold props */
	obs_property_t *p = obs_properties_add_bool(
		props, "enable_threshold", obs_module_text("EnableThreshold"));
	obs_property_set_modified_callback(p, enable_threshold_modified);

	obs_properties_add_float_slider(props, "threshold",
					obs_module_text("Threshold"), 0.0, 1.0,
					0.025);

	obs_properties_add_float_slider(
		props, "contour_filter",
		obs_module_text("ContourFilterPercentOfImage"), 0.0, 1.0,
		0.025);

	obs_properties_add_float_slider(props, "smooth_contour",
					obs_module_text("SmoothSilhouette"),
					0.0, 1.0, 0.05);

	obs_properties_add_float_slider(
		props, "feather", obs_module_text("FeatherBlendSilhouette"),
		0.0, 1.0, 0.05);

	/* GPU, CPU and performance Props */
	obs_property_t *p_use_gpu = obs_properties_add_list(
		props, "useGPU", obs_module_text("InferenceDevice"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p_use_gpu, obs_module_text("CPU"),
				     USEGPU_CPU);
#if defined(__linux__) && defined(__x86_64__)
	obs_property_list_add_string(p_use_gpu, obs_module_text("GPUTensorRT"),
				     USEGPU_TENSORRT);
#endif
#if _WIN32
	obs_property_list_add_string(p_use_gpu, obs_module_text("GPUDirectML"),
				     USEGPU_DML);
#endif
#if defined(__APPLE__)
	obs_property_list_add_string(p_use_gpu, obs_module_text("CoreML"),
				     USEGPU_COREML);
#endif

	obs_properties_add_int(props, "mask_every_x_frames",
			       obs_module_text("CalculateMaskEveryXFrame"), 1,
			       300, 1);
	obs_properties_add_int_slider(props, "numThreads",
				      obs_module_text("NumThreads"), 0, 8, 1);

	/* Model selection Props */
	obs_property_t *p_model_select = obs_properties_add_list(
		props, "model_select", obs_module_text("SegmentationModel"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p_model_select, obs_module_text("SINet"),
				     MODEL_SINET);
	obs_property_list_add_string(
		p_model_select, obs_module_text("MediaPipe"), MODEL_MEDIAPIPE);
	obs_property_list_add_string(p_model_select,
				     obs_module_text("Selfie Segmentation"),
				     MODEL_SELFIE);
	obs_property_list_add_string(p_model_select,
				     obs_module_text("PPHumanSeg"),
				     MODEL_PPHUMANSEG);
	obs_property_list_add_string(p_model_select,
				     obs_module_text("Robust Video Matting"),
				     MODEL_RVM);
	obs_property_list_add_string(p_model_select,
				     obs_module_text("TCMonoDepth"),
				     MODEL_DEPTH_TCMONODEPTH);

	/* Background Blur Props */
	obs_properties_add_int_slider(
		props, "blur_background",
		obs_module_text("BlurBackgroundFactor0NoBlurUseColor"), 0, 20,
		1);

	obs_properties_add_float_slider(props, "blur_focus_point",
					obs_module_text("BlurFocusPoint"), 0.0,
					1.0, 0.05);

	UNUSED_PARAMETER(data);
	return props;
}

void background_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enable_threshold", true);
	obs_data_set_default_double(settings, "threshold", 0.5);
	obs_data_set_default_double(settings, "contour_filter", 0.05);
	obs_data_set_default_double(settings, "smooth_contour", 0.5);
	obs_data_set_default_double(settings, "feather", 0.0);
#if _WIN32
	obs_data_set_default_string(settings, "useGPU", USEGPU_DML);
#elif defined(__APPLE__)
	obs_data_set_default_string(settings, "useGPU", USEGPU_CPU);
#else
	// Linux
	obs_data_set_default_string(settings, "useGPU", USEGPU_CPU);
#endif
	obs_data_set_default_string(settings, "model_select", MODEL_MEDIAPIPE);
	obs_data_set_default_int(settings, "mask_every_x_frames", 1);
	obs_data_set_default_int(settings, "blur_background", 0);
	obs_data_set_default_int(settings, "numThreads", 1);
	obs_data_set_default_double(settings, "blur_focus_point", 0.1);
}

void background_filter_update(void *data, obs_data_t *settings)
{
	struct background_removal_filter *tf =
		reinterpret_cast<background_removal_filter *>(data);
	tf->enableThreshold =
		(float)obs_data_get_bool(settings, "enable_threshold");
	tf->threshold = (float)obs_data_get_double(settings, "threshold");

	tf->contourFilter =
		(float)obs_data_get_double(settings, "contour_filter");
	tf->smoothContour =
		(float)obs_data_get_double(settings, "smooth_contour");
	tf->feather = (float)obs_data_get_double(settings, "feather");
	tf->maskEveryXFrames =
		(int)obs_data_get_int(settings, "mask_every_x_frames");
	tf->maskEveryXFramesCount = (int)(0);
	tf->blurBackground = obs_data_get_int(settings, "blur_background");
	tf->blurFocusPoint =
		(float)obs_data_get_double(settings, "blur_focus_point");

	const std::string newUseGpu = obs_data_get_string(settings, "useGPU");
	const std::string newModel =
		obs_data_get_string(settings, "model_select");
	const uint32_t newNumThreads =
		(uint32_t)obs_data_get_int(settings, "numThreads");

	if (tf->modelSelection.empty() || tf->modelSelection != newModel ||
	    tf->useGPU != newUseGpu || tf->numThreads != newNumThreads) {
		// Re-initialize model if it's not already the selected one or switching inference device
		tf->modelSelection = newModel;
		tf->useGPU = newUseGpu;
		tf->numThreads = newNumThreads;

		if (tf->modelSelection == MODEL_SINET) {
			tf->model.reset(new ModelSINET);
		}
		if (tf->modelSelection == MODEL_SELFIE) {
			tf->model.reset(new ModelSelfie);
		}
		if (tf->modelSelection == MODEL_MEDIAPIPE) {
			tf->model.reset(new ModelMediaPipe);
		}
		if (tf->modelSelection == MODEL_RVM) {
			tf->model.reset(new ModelRVM);
		}
		if (tf->modelSelection == MODEL_PPHUMANSEG) {
			tf->model.reset(new ModelPPHumanSeg);
		}
		if (tf->modelSelection == MODEL_DEPTH_TCMONODEPTH) {
			tf->model.reset(new ModelTCMonoDepth);
		}

		createOrtSession(tf);
	}

	obs_enter_graphics();

	char *effect_path = obs_module_file(EFFECT_PATH);
	gs_effect_destroy(tf->effect);
	tf->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);

	char *kawaseBlurEffectPath = obs_module_file(KAWASE_BLUR_EFFECT_PATH);
	gs_effect_destroy(tf->kawaseBlurEffect);
	tf->kawaseBlurEffect =
		gs_effect_create_from_file(kawaseBlurEffectPath, NULL);
	bfree(kawaseBlurEffectPath);

	obs_leave_graphics();
}

void background_filter_activate(void *data)
{
	struct background_removal_filter *tf =
		reinterpret_cast<background_removal_filter *>(data);
	tf->isDisabled = false;
	send_segment_trace(SEGMENT_TYPE_FILTER_ACTIVATED, -1);
}

void background_filter_deactivate(void *data)
{
	struct background_removal_filter *tf =
		reinterpret_cast<background_removal_filter *>(data);
	tf->isDisabled = true;
	send_segment_trace(SEGMENT_TYPE_FILTER_DEACTIVATED, -1);
}

/**                   FILTER CORE                     */

void *background_filter_create(obs_data_t *settings, obs_source_t *source)
{
	void *data = bmalloc(sizeof(struct background_removal_filter));
	struct background_removal_filter *tf = new (data)
		background_removal_filter();

	send_segment_trace(SEGMENT_TYPE_FILTER_CREATE, -1);

	tf->source = source;
	tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

	std::string instanceName{"background-removal-inference"};
	tf->env.reset(new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR,
				   instanceName.c_str()));

	tf->modelSelection = MODEL_MEDIAPIPE;
	background_filter_update(tf, settings);

	return tf;
}

void background_filter_destroy(void *data)
{
	struct background_removal_filter *tf =
		reinterpret_cast<background_removal_filter *>(data);

	send_segment_trace(SEGMENT_TYPE_FILTER_DESTROY, -1);

	if (tf) {
		obs_enter_graphics();
		gs_texrender_destroy(tf->texrender);
		if (tf->stagesurface) {
			gs_stagesurface_destroy(tf->stagesurface);
		}
		gs_effect_destroy(tf->effect);
		gs_effect_destroy(tf->kawaseBlurEffect);
		obs_leave_graphics();

		tf->~background_removal_filter();
		bfree(tf);
	}
}

static void processImageForBackground(struct background_removal_filter *tf,
				      const cv::Mat &imageBGRA,
				      cv::Mat &backgroundMask)
{
	cv::Mat outputImage;
	if (!runFilterModelInference(tf, imageBGRA, outputImage)) {
		return;
	}
	// Assume outputImage is now a single channel, uint8 image with values between 0 and 255

	// If we have a threshold, apply it. Otherwise, just use the output image as the mask
	if (tf->enableThreshold) {
		// We need to make tf->threshold (float [0,1]) be in that range
		const uint8_t threshold_value =
			(uint8_t)(tf->threshold * 255.0f);
		backgroundMask = outputImage < threshold_value;
	} else {
		backgroundMask = 255 - outputImage;
	}
}

void background_filter_video_tick(void *data, float seconds)
{
	struct background_removal_filter *tf =
		reinterpret_cast<background_removal_filter *>(data);

	if (tf->isDisabled) {
		return;
	}

	if (!obs_source_enabled(tf->source)) {
		return;
	}

	if (tf->inputBGRA.empty()) {
		return;
	}

	cv::Mat imageBGRA;
	{
		std::unique_lock<std::mutex> lock(tf->inputBGRALock,
						  std::try_to_lock);
		if (!lock.owns_lock()) {
			return;
		}
		imageBGRA = tf->inputBGRA.clone();
	}

	if (tf->backgroundMask.empty()) {
		// First frame. Initialize the background mask.
		tf->backgroundMask =
			cv::Mat(imageBGRA.size(), CV_8UC1, cv::Scalar(255));
	}

	tf->maskEveryXFramesCount++;
	tf->maskEveryXFramesCount %= tf->maskEveryXFrames;

	try {
		if (tf->maskEveryXFramesCount != 0 &&
		    !tf->backgroundMask.empty()) {
			// We are skipping processing of the mask for this frame.
			// Get the background mask previously generated.
			; // Do nothing
		} else {
			cv::Mat backgroundMask;

			// Process the image to find the mask.
			processImageForBackground(tf, imageBGRA,
						  backgroundMask);

			// Contour processing
			// Only applicable if we are thresholding (and get a binary image)
			if (tf->enableThreshold) {
				if (tf->contourFilter > 0.0 &&
				    tf->contourFilter < 1.0) {
					std::vector<std::vector<cv::Point>>
						contours;
					findContours(backgroundMask, contours,
						     cv::RETR_EXTERNAL,
						     cv::CHAIN_APPROX_SIMPLE);
					std::vector<std::vector<cv::Point>>
						filteredContours;
					const double contourSizeThreshold =
						(double)(backgroundMask.total()) *
						tf->contourFilter;
					for (auto &contour : contours) {
						if (cv::contourArea(contour) >
						    (double)contourSizeThreshold) {
							filteredContours
								.push_back(
									contour);
						}
					}
					backgroundMask.setTo(0);
					drawContours(backgroundMask,
						     filteredContours, -1,
						     cv::Scalar(255), -1);
				}

				if (tf->smoothContour > 0.0) {
					int k_size =
						(int)(3 +
						      11 * tf->smoothContour);
					k_size += k_size % 2 == 0 ? 1 : 0;
					cv::stackBlur(backgroundMask,
						      backgroundMask,
						      cv::Size(k_size, k_size));
				}

				// Resize the size of the mask back to the size of the original input.
				cv::resize(backgroundMask, backgroundMask,
					   imageBGRA.size());

				// Additional contour processing at full resolution
				if (tf->smoothContour > 0.0) {
					// If the mask was smoothed, apply a threshold to get a binary mask
					backgroundMask = backgroundMask > 128;
				}

				if (tf->feather > 0.0) {
					// Feather (blur) the mask
					int k_size = (int)(40 * tf->feather);
					k_size += k_size % 2 == 0 ? 1 : 0;
					cv::dilate(backgroundMask,
						   backgroundMask, cv::Mat(),
						   cv::Point(-1, -1),
						   k_size / 3);
					cv::boxFilter(
						backgroundMask, backgroundMask,
						tf->backgroundMask.depth(),
						cv::Size(k_size, k_size));
				}
			}

			// Save the mask for the next frame
			backgroundMask.copyTo(tf->backgroundMask);
		}
	} catch (const Ort::Exception &e) {
		blog(LOG_ERROR, "ONNXRuntime Exception: %s", e.what());
		send_segment_trace(SEGMENT_TYPE_FILTER_ERROR, -1);
		// TODO: Fall back to CPU if it makes sense
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "%s", e.what());
		send_segment_trace(SEGMENT_TYPE_FILTER_ERROR, -1);
	}

	UNUSED_PARAMETER(seconds);
}

static gs_texture_t *blur_background(struct background_removal_filter *tf,
				     uint32_t width, uint32_t height,
				     gs_texture_t *alphaTexture)
{
	if (tf->blurBackground == 0 || !tf->kawaseBlurEffect) {
		return nullptr;
	}
	gs_texture_t *blurredTexture =
		gs_texture_create(width, height, GS_BGRA, 1, nullptr, 0);
	gs_copy_texture(blurredTexture,
			gs_texrender_get_texture(tf->texrender));
	gs_eparam_t *image =
		gs_effect_get_param_by_name(tf->kawaseBlurEffect, "image");
	gs_eparam_t *focalmask =
		gs_effect_get_param_by_name(tf->kawaseBlurEffect, "focalmask");
	gs_eparam_t *xOffset =
		gs_effect_get_param_by_name(tf->kawaseBlurEffect, "xOffset");
	gs_eparam_t *yOffset =
		gs_effect_get_param_by_name(tf->kawaseBlurEffect, "yOffset");
	gs_eparam_t *blurIter =
		gs_effect_get_param_by_name(tf->kawaseBlurEffect, "blurIter");
	gs_eparam_t *blurTotal =
		gs_effect_get_param_by_name(tf->kawaseBlurEffect, "blurTotal");
	gs_eparam_t *blurFocusPointParam = gs_effect_get_param_by_name(
		tf->kawaseBlurEffect, "blurFocusPoint");

	for (int i = 0; i < (int)tf->blurBackground; i++) {
		gs_texrender_reset(tf->texrender);
		if (!gs_texrender_begin(tf->texrender, width, height)) {
			blog(LOG_INFO,
			     "Could not open background blur texrender!");
			return blurredTexture;
		}

		gs_effect_set_texture(image, blurredTexture);
		gs_effect_set_texture(focalmask, alphaTexture);
		gs_effect_set_float(xOffset, ((float)i + 0.5f) / (float)width);
		gs_effect_set_float(yOffset, ((float)i + 0.5f) / (float)height);
		gs_effect_set_int(blurIter, i);
		gs_effect_set_int(blurTotal, (int)tf->blurBackground);
		gs_effect_set_float(blurFocusPointParam, tf->blurFocusPoint);

		struct vec4 background;
		vec4_zero(&background);
		gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f,
			 static_cast<float>(height), -100.0f, 100.0f);
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		while (gs_effect_loop(tf->kawaseBlurEffect, "Draw")) {
			gs_draw_sprite(blurredTexture, 0, width, height);
		}
		gs_blend_state_pop();
		gs_texrender_end(tf->texrender);
		gs_copy_texture(blurredTexture,
				gs_texrender_get_texture(tf->texrender));
	}
	return blurredTexture;
}

void background_filter_video_render(void *data, gs_effect_t *_effect)
{
	UNUSED_PARAMETER(_effect);

	struct background_removal_filter *tf =
		reinterpret_cast<background_removal_filter *>(data);

	uint32_t width, height;
	if (!getRGBAFromStageSurface(tf, width, height)) {
		obs_source_skip_video_filter(tf->source);
		return;
	}

	if (!tf->effect) {
		// Effect failed to load, skip rendering
		obs_source_skip_video_filter(tf->source);
		return;
	}

	gs_texture_t *alphaTexture = nullptr;
	{
		std::lock_guard<std::mutex> lock(tf->outputLock);
		alphaTexture = gs_texture_create(
			tf->backgroundMask.cols, tf->backgroundMask.rows, GS_R8,
			1, (const uint8_t **)&tf->backgroundMask.data, 0);
		if (!alphaTexture) {
			blog(LOG_ERROR, "Failed to create alpha texture");
			obs_source_skip_video_filter(tf->source);
			send_segment_trace(SEGMENT_TYPE_FILTER_ERROR, -1);
			return;
		}
	}

	// Output the masked image
	gs_texture_t *blurredTexture =
		blur_background(tf, width, height, alphaTexture);

	if (!obs_source_process_filter_begin(tf->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING)) {
		obs_source_skip_video_filter(tf->source);
		gs_texture_destroy(alphaTexture);
		gs_texture_destroy(blurredTexture);
		return;
	}

	gs_eparam_t *alphamask =
		gs_effect_get_param_by_name(tf->effect, "alphamask");
	gs_eparam_t *blurredBackground =
		gs_effect_get_param_by_name(tf->effect, "blurredBackground");

	gs_effect_set_texture(alphamask, alphaTexture);

	if (tf->blurBackground > 0) {
		gs_effect_set_texture(blurredBackground, blurredTexture);
	}

	gs_blend_state_push();
	gs_reset_blend_state();

	const char *techName;
	if (tf->blurBackground > 0) {
		techName = "DrawWithBlur";
	} else {
		techName = "DrawWithoutBlur";
	}

	obs_source_process_filter_tech_end(tf->source, tf->effect, 0, 0,
					   techName);

	gs_blend_state_pop();

	gs_texture_destroy(alphaTexture);
	gs_texture_destroy(blurredTexture);
}

void background_filter_remove(void *data, obs_source_t *source) {
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(data);

	send_segment_trace(SEGMENT_TYPE_FILTER_REMOVE, -1);
}
