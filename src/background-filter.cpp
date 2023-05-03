#include <obs-module.h>

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

#include "plugin-macros.generated.h"
#include "models/ModelSINET.h"
#include "models/ModelMediapipe.h"
#include "models/ModelSelfie.h"
#include "models/ModelRVM.h"
#include "models/ModelPPHumanSeg.h"
#include "FilterData.h"
#include "ort-utils/ort-session-utils.h"
#include "obs-utils/obs-utils.h"
#include "consts.h"

struct background_removal_filter : public filter_data {
  float threshold = 0.5f;
  cv::Scalar backgroundColor{0, 0, 0, 0};
  float contourFilter = 0.05f;
  float smoothContour = 0.5f;
  float feather = 0.0f;

  cv::Mat backgroundMask;
  int maskEveryXFrames = 1;
  int maskEveryXFramesCount = 0;
  int64_t blurBackground = 0;

  gs_effect_t *effect;
  gs_effect_t *kawaseBlurEffect;
};

static const char *filter_getname(void *unused)
{
  UNUSED_PARAMETER(unused);
  return obs_module_text("BackgroundRemoval");
}

/**                   PROPERTIES                     */

static obs_properties_t *filter_properties(void *data)
{
  obs_properties_t *props = obs_properties_create();

  obs_properties_add_float_slider(props, "threshold", obs_module_text("Threshold"), 0.0, 1.0,
                                  0.025);

  obs_properties_add_float_slider(props, "contour_filter",
                                  obs_module_text("ContourFilterPercentOfImage"), 0.0, 1.0, 0.025);

  obs_properties_add_float_slider(props, "smooth_contour", obs_module_text("SmoothSilhouette"), 0.0,
                                  1.0, 0.05);

  obs_properties_add_float_slider(props, "feather", obs_module_text("FeatherBlendSilhouette"), 0.0,
                                  1.0, 0.05);

  obs_property_t *p_use_gpu = obs_properties_add_list(props, "useGPU",
                                                      obs_module_text("InferenceDevice"),
                                                      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

  obs_property_list_add_string(p_use_gpu, obs_module_text("CPU"), USEGPU_CPU);
#ifdef __linux__
  obs_property_list_add_string(p_use_gpu, obs_module_text("GPUTensorRT"), USEGPU_TENSORRT);
#endif
#if _WIN32
  obs_property_list_add_string(p_use_gpu, obs_module_text("GPUDirectML"), USEGPU_DML);
#endif
#if defined(__APPLE__)
  obs_property_list_add_string(p_use_gpu, obs_module_text("CoreML"), USEGPU_COREML);
#endif

  obs_property_t *p_model_select =
    obs_properties_add_list(props, "model_select", obs_module_text("SegmentationModel"),
                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

  obs_property_list_add_string(p_model_select, obs_module_text("SINet"), MODEL_SINET);
  obs_property_list_add_string(p_model_select, obs_module_text("MediaPipe"), MODEL_MEDIAPIPE);
  obs_property_list_add_string(p_model_select, obs_module_text("Selfie Segmentation"),
                               MODEL_SELFIE);
  obs_property_list_add_string(p_model_select, obs_module_text("PPHumanSeg"), MODEL_PPHUMANSEG);
  obs_property_list_add_string(p_model_select, obs_module_text("Robust Video Matting"), MODEL_RVM);

  obs_properties_add_int(props, "mask_every_x_frames", obs_module_text("CalculateMaskEveryXFrame"),
                         1, 300, 1);

  obs_properties_add_int_slider(props, "blur_background",
                                obs_module_text("BlurBackgroundFactor0NoBlurUseColor"), 0, 20, 1);
  obs_properties_add_int_slider(props, "numThreads", obs_module_text("NumThreads"), 0, 8, 1);

  UNUSED_PARAMETER(data);
  return props;
}

static void filter_defaults(obs_data_t *settings)
{
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
}

static void filter_update(void *data, obs_data_t *settings)
{
  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);
  tf->threshold = (float)obs_data_get_double(settings, "threshold");

  tf->contourFilter = (float)obs_data_get_double(settings, "contour_filter");
  tf->smoothContour = (float)obs_data_get_double(settings, "smooth_contour");
  tf->feather = (float)obs_data_get_double(settings, "feather");
  tf->maskEveryXFrames = (int)obs_data_get_int(settings, "mask_every_x_frames");
  tf->maskEveryXFrames = (int)obs_data_get_int(settings, "mask_every_x_frames");
  tf->maskEveryXFramesCount = (int)(0);
  tf->blurBackground = obs_data_get_int(settings, "blur_background");

  const std::string newUseGpu = obs_data_get_string(settings, "useGPU");
  const std::string newModel = obs_data_get_string(settings, "model_select");
  const uint32_t newNumThreads = (uint32_t)obs_data_get_int(settings, "numThreads");

  if (tf->modelSelection.empty() || tf->modelSelection != newModel || tf->useGPU != newUseGpu ||
      tf->numThreads != newNumThreads) {
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

    createOrtSession(tf);
  }

  obs_enter_graphics();

  char *effect_path = obs_module_file(EFFECT_PATH);
  gs_effect_destroy(tf->effect);
  tf->effect = gs_effect_create_from_file(effect_path, NULL);
  bfree(effect_path);

  char *kawaseBlurEffectPath = obs_module_file(KAWASE_BLUR_EFFECT_PATH);
  gs_effect_destroy(tf->kawaseBlurEffect);
  tf->kawaseBlurEffect = gs_effect_create_from_file(kawaseBlurEffectPath, NULL);
  bfree(kawaseBlurEffectPath);

  obs_leave_graphics();
}

static void filter_activate(void *data)
{
  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);
  tf->isDisabled = false;
}

static void filter_deactivate(void *data)
{
  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);
  tf->isDisabled = true;
}

/**                   FILTER CORE                     */

static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
  void *data = bmalloc(sizeof(struct background_removal_filter));
  struct background_removal_filter *tf = new (data) background_removal_filter();

  tf->source = source;
  tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

  std::string instanceName{"background-removal-inference"};
  tf->env.reset(new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, instanceName.c_str()));

  tf->modelSelection = MODEL_MEDIAPIPE;
  filter_update(tf, settings);

  return tf;
}

static void filter_destroy(void *data)
{
  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

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
                                      const cv::Mat &imageBGRA, cv::Mat &backgroundMask)
{
  cv::Mat outputImage;
  if (!runFilterModelInference(tf, imageBGRA, outputImage)) {
    return;
  }

  if (tf->modelSelection == MODEL_SINET || tf->modelSelection == MODEL_MEDIAPIPE) {
    backgroundMask = outputImage > tf->threshold;
  } else {
    backgroundMask = outputImage < tf->threshold;
  }
}

void filter_video_tick(void *data, float seconds)
{
  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

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
    std::unique_lock<std::mutex> lock(tf->inputBGRALock, std::try_to_lock);
    if (!lock.owns_lock()) {
      return;
    }
    imageBGRA = tf->inputBGRA.clone();
  }

  if (tf->backgroundMask.empty()) {
    // First frame. Initialize the background mask.
    tf->backgroundMask = cv::Mat(imageBGRA.size(), CV_8UC1, cv::Scalar(255));
  }

  tf->maskEveryXFramesCount++;
  tf->maskEveryXFramesCount %= tf->maskEveryXFrames;

  try {
    if (tf->maskEveryXFramesCount != 0 && !tf->backgroundMask.empty()) {
      // We are skipping processing of the mask for this frame.
      // Get the background mask previously generated.
      ; // Do nothing
    } else {
      cv::Mat backgroundMask;

      // Process the image to find the mask.
      processImageForBackground(tf, imageBGRA, backgroundMask);

      // Contour processing
      if (tf->contourFilter > 0.0 && tf->contourFilter < 1.0) {
        std::vector<std::vector<cv::Point>> contours;
        findContours(backgroundMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        std::vector<std::vector<cv::Point>> filteredContours;
        const int64_t contourSizeThreshold = (int64_t)(backgroundMask.total() * tf->contourFilter);
        for (auto &contour : contours) {
          if (cv::contourArea(contour) > contourSizeThreshold) {
            filteredContours.push_back(contour);
          }
        }
        backgroundMask.setTo(0);
        drawContours(backgroundMask, filteredContours, -1, cv::Scalar(255), -1);
      }

      if (tf->smoothContour > 0.0) {
        int k_size = (int)(3 + 11 * tf->smoothContour);
        k_size += k_size % 2 == 0 ? 1 : 0;
        cv::stackBlur(backgroundMask, backgroundMask, cv::Size(k_size, k_size));
      }

      // Resize the size of the mask back to the size of the original input.
      cv::resize(backgroundMask, backgroundMask, imageBGRA.size());

      if (tf->smoothContour > 0.0) {
        // If the mask was smoothed, apply a threshold to get a binary mask
        backgroundMask = backgroundMask > 128;
      }

      if (tf->feather > 0.0) {
        // Feather (blur) the mask
        int k_size = (int)(40 * tf->feather);
        k_size += k_size % 2 == 0 ? 1 : 0;
        cv::dilate(backgroundMask, backgroundMask, cv::Mat(), cv::Point(-1, -1), k_size / 3);
        cv::boxFilter(backgroundMask, backgroundMask, tf->backgroundMask.depth(),
                      cv::Size(k_size, k_size));
      }

      // Save the mask for the next frame
      backgroundMask.copyTo(tf->backgroundMask);
    }
  } catch (const Ort::Exception &e) {
    blog(LOG_ERROR, "ONNXRuntime Exception: %s", e.what());
    // TODO: Fall back to CPU if it makes sense
  } catch (const std::exception &e) {
    blog(LOG_ERROR, "%s", e.what());
  }

  UNUSED_PARAMETER(seconds);
}

static gs_texture_t *blur_background(struct background_removal_filter *tf, uint32_t width,
                                     uint32_t height)
{
  if (tf->blurBackground == 0.0 || !tf->kawaseBlurEffect) {
    return nullptr;
  }
  gs_texture_t *blurredTexture = gs_texture_create(width, height, GS_BGRA, 1, nullptr, 0);
  gs_copy_texture(blurredTexture, gs_texrender_get_texture(tf->texrender));
  gs_eparam_t *image = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "image");
  gs_eparam_t *xOffset = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "xOffset");
  gs_eparam_t *yOffset = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "yOffset");

  for (int i = 0; i < (int)tf->blurBackground; i++) {
    gs_texrender_reset(tf->texrender);
    if (!gs_texrender_begin(tf->texrender, width, height)) {
      blog(LOG_INFO, "Could not open background blur texrender!");
      return blurredTexture;
    }

    gs_effect_set_texture(image, blurredTexture);
    gs_effect_set_float(xOffset, (i + 0.5f) / width);
    gs_effect_set_float(yOffset, (i + 0.5f) / height);

    struct vec4 background;
    vec4_zero(&background);
    gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
    gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
    while (gs_effect_loop(tf->kawaseBlurEffect, "Draw")) {
      gs_draw_sprite(blurredTexture, 0, width, height);
    }
    gs_blend_state_pop();
    gs_texrender_end(tf->texrender);
    gs_copy_texture(blurredTexture, gs_texrender_get_texture(tf->texrender));
  }
  return blurredTexture;
}

static void filter_video_render(void *data, gs_effect_t *_effect)
{
  UNUSED_PARAMETER(_effect);

  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

  uint32_t width, height;
  if (!getRGBAFromStageSurface(tf, width, height)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }

  // Output the masked image

  gs_texture_t *blurredTexture = blur_background(tf, width, height);

  if (!tf->effect) {
    // Effect failed to load, skip rendering
    obs_source_skip_video_filter(tf->source);
    return;
  }

  if (!obs_source_process_filter_begin(tf->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }

  gs_texture_t *alphaTexture = nullptr;
  {
    std::lock_guard<std::mutex> lock(tf->outputLock);
    alphaTexture = gs_texture_create(tf->backgroundMask.cols, tf->backgroundMask.rows, GS_R8, 1,
                                     (const uint8_t **)&tf->backgroundMask.data, 0);
    if (!alphaTexture) {
      blog(LOG_ERROR, "Failed to create alpha texture");
      obs_source_skip_video_filter(tf->source);
      return;
    }
  }
  gs_eparam_t *alphamask = gs_effect_get_param_by_name(tf->effect, "alphamask");
  gs_eparam_t *blurSize = gs_effect_get_param_by_name(tf->effect, "blurSize");
  gs_eparam_t *xTexelSize = gs_effect_get_param_by_name(tf->effect, "xTexelSize");
  gs_eparam_t *yTexelSize = gs_effect_get_param_by_name(tf->effect, "yTexelSize");
  gs_eparam_t *blurredBackground = gs_effect_get_param_by_name(tf->effect, "blurredBackground");

  gs_effect_set_texture(alphamask, alphaTexture);
  gs_effect_set_int(blurSize, (int)tf->blurBackground);
  gs_effect_set_float(xTexelSize, 1.0f / width);
  gs_effect_set_float(yTexelSize, 1.0f / height);
  if (tf->blurBackground > 0.0) {
    gs_effect_set_texture(blurredBackground, blurredTexture);
  }

  gs_blend_state_push();
  gs_reset_blend_state();

  const char *techName;
  if (tf->blurBackground > 0.0) {
    techName = "DrawWithBlur";
  } else {
    techName = "DrawWithoutBlur";
  }

  obs_source_process_filter_tech_end(tf->source, tf->effect, 0, 0, techName);

  gs_blend_state_pop();

  gs_texture_destroy(alphaTexture);
  gs_texture_destroy(blurredTexture);
}

struct obs_source_info background_removal_filter_info = {
  .id = "background_removal",
  .type = OBS_SOURCE_TYPE_FILTER,
  .output_flags = OBS_SOURCE_VIDEO,
  .get_name = filter_getname,
  .create = filter_create,
  .destroy = filter_destroy,
  .get_defaults = filter_defaults,
  .get_properties = filter_properties,
  .update = filter_update,
  .activate = filter_activate,
  .deactivate = filter_deactivate,
  .video_tick = filter_video_tick,
  .video_render = filter_video_render,
};
