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
#include "consts.h"
#include "obs-utils/obs-utils.h"
#include "ort-utils/ort-session-utils.h"
#include "models/ModelTBEFN.h"
#include "models/ModelZeroDCE.h"
#include "models/ModelURetinex.h"

struct enhance_filter : public filter_data {
  cv::Mat outputBGRA;
  gs_effect_t *blendEffect;
  float blendFactor;
};

static const char *filter_getname(void *unused)
{
  UNUSED_PARAMETER(unused);
  return obs_module_text("EnhancePortrait");
}

static obs_properties_t *filter_properties(void *data)
{
  UNUSED_PARAMETER(data);
  obs_properties_t *props = obs_properties_create();
  obs_properties_add_float_slider(props, "blend", obs_module_text("EffectStrengh"), 0.0, 1.0, 0.05);
  obs_properties_add_int_slider(props, "numThreads", obs_module_text("NumThreads"), 0, 8, 1);
  obs_property_t *p_model_select =
    obs_properties_add_list(props, "model_select", obs_module_text("EnhancementModel"),
                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

  obs_property_list_add_string(p_model_select, obs_module_text("TBEFN"), MODEL_ENHANCE_TBEFN);
  obs_property_list_add_string(p_model_select, obs_module_text("URETINEX"), MODEL_ENHANCE_URETINEX);
  obs_property_list_add_string(p_model_select, obs_module_text("SGLLIE"), MODEL_ENHANCE_SGLLIE);
  obs_property_list_add_string(p_model_select, obs_module_text("ZERODCE"), MODEL_ENHANCE_ZERODCE);
  return props;
}

static void filter_defaults(obs_data_t *settings)
{
  obs_data_set_default_double(settings, "blend", 1.0);
  obs_data_set_default_int(settings, "numThreads", 1);
  obs_data_set_default_string(settings, "model_select", MODEL_ENHANCE_TBEFN);
}

static void filter_activate(void *data)
{
  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);
  tf->isDisabled = false;
}

static void filter_deactivate(void *data)
{
  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);
  tf->isDisabled = true;
}

static void filter_update(void *data, obs_data_t *settings)
{
  UNUSED_PARAMETER(settings);
  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);

  tf->blendFactor = (float)obs_data_get_double(settings, "blend");
  const uint32_t newNumThreads = (uint32_t)obs_data_get_int(settings, "numThreads");
  const std::string newModel = obs_data_get_string(settings, "model_select");

  if (tf->modelSelection.empty() || tf->modelSelection != newModel ||
      tf->numThreads != newNumThreads) {
    tf->numThreads = newNumThreads;
    tf->modelSelection = newModel;
    if (tf->modelSelection == MODEL_ENHANCE_TBEFN) {
      tf->model.reset(new ModelTBEFN);
    } else if (tf->modelSelection == MODEL_ENHANCE_ZERODCE) {
      tf->model.reset(new ModelZeroDCE);
    } else if (tf->modelSelection == MODEL_ENHANCE_URETINEX) {
      tf->model.reset(new ModelURetinex);
    } else {
      tf->model.reset(new ModelBCHW);
    }
    tf->useGPU = USEGPU_CPU;
    createOrtSession(tf);
  }

  if (tf->blendEffect == nullptr) {
    obs_enter_graphics();

    char *effect_path = obs_module_file(BLEND_EFFECT_PATH);
    gs_effect_destroy(tf->blendEffect);
    tf->blendEffect = gs_effect_create_from_file(effect_path, NULL);
    bfree(effect_path);

    obs_leave_graphics();
  }
}

static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
  void *data = bmalloc(sizeof(struct enhance_filter));
  struct enhance_filter *tf = new (data) enhance_filter();

  tf->source = source;
  tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

  std::string instanceName{"enhance-portrait-inference"};
  tf->env.reset(new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, instanceName.c_str()));

  filter_update(tf, settings);

  return tf;
}

static void filter_destroy(void *data)
{
  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);

  if (tf) {
    obs_enter_graphics();
    gs_texrender_destroy(tf->texrender);
    if (tf->stagesurface) {
      gs_stagesurface_destroy(tf->stagesurface);
    }
    obs_leave_graphics();
    tf->~enhance_filter();
    bfree(tf);
  }
}

static void filter_video_tick(void *data, float seconds)
{
  UNUSED_PARAMETER(seconds);
  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);

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
    std::unique_lock<std::mutex> lock(tf->inputBGRALock, std::try_to_lock);
    if (!lock.owns_lock()) {
      return;
    }
    imageBGRA = tf->inputBGRA.clone();
  }

  cv::Mat outputImage;
  try {
    if (!runFilterModelInference(tf, imageBGRA, outputImage)) {
      return;
    }
  } catch (const std::exception &e) {
    blog(LOG_ERROR, "Exception caught: %s", e.what());
    return;
  }

  // Put output image back to source rendering pipeline
  {
    std::unique_lock<std::mutex> lock(tf->outputLock, std::try_to_lock);
    if (!lock.owns_lock()) {
      return;
    }

    // TODO add high pass filter from original image to output image
    // to reduce the blur effect of the small network output

    cv::cvtColor(outputImage, tf->outputBGRA, cv::COLOR_BGR2RGBA);
  }
}

static void filter_video_render(void *data, gs_effect_t *_effect)
{
  UNUSED_PARAMETER(_effect);

  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);

  // Get input from source
  uint32_t width, height;
  if (!getRGBAFromStageSurface(tf, width, height)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }

  // Engage filter
  if (!obs_source_process_filter_begin(tf->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }

  // Get output from neural network into texture
  gs_texture_t *outputTexture = nullptr;
  {
    std::lock_guard<std::mutex> lock(tf->outputLock);
    outputTexture = gs_texture_create(tf->outputBGRA.cols, tf->outputBGRA.rows, GS_BGRA, 1,
                                      (const uint8_t **)&tf->outputBGRA.data, 0);
    if (!outputTexture) {
      blog(LOG_ERROR, "Failed to create output texture");
      obs_source_skip_video_filter(tf->source);
      return;
    }
  }

  gs_eparam_t *blendimage = gs_effect_get_param_by_name(tf->blendEffect, "blendimage");
  gs_eparam_t *blendFactor = gs_effect_get_param_by_name(tf->blendEffect, "blendFactor");

  gs_effect_set_texture(blendimage, outputTexture);
  gs_effect_set_float(blendFactor, tf->blendFactor);

  // Render texture
  gs_blend_state_push();
  gs_reset_blend_state();

  obs_source_process_filter_tech_end(tf->source, tf->blendEffect, 0, 0, "Draw");

  // while (gs_effect_loop(tf->blendEffect, "Draw")) {
  //   obs_source_draw(outputTexture, 0, 0, width, height, false);
  // }

  gs_blend_state_pop();

  gs_texture_destroy(outputTexture);
}

struct obs_source_info enhance_filter_info = {
  .id = "enhanceportrait",
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
