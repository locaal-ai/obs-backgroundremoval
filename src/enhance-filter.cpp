#include <obs-module.h>

#include <onnxruntime_cxx_api.h>
#include <cpu_provider_factory.h>

#if defined(__APPLE__)
#include <coreml_provider_factory.h>
#endif

#ifdef __linux__
#include <tensorrt_provider_factory.h>
#endif

#ifdef _WIN32
#include <dml_provider_factory.h>
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

struct enhance_filter : public filter_data {
  cv::Mat outputBGRA;
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
  return props;
}

static void filter_defaults(obs_data_t *settings)
{
  UNUSED_PARAMETER(settings);
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

  if (tf->modelSelection.empty()) {
    tf->modelSelection = MODEL_ENHANCE;
    tf->model.reset(new ModelBCHW);
    createOrtSession(tf);
  }

  // obs_enter_graphics();

  // char *effect_path = obs_module_file(EFFECT_PATH);
  // gs_effect_destroy(tf->effect);
  // tf->effect = gs_effect_create_from_file(effect_path, NULL);
  // bfree(effect_path);

  // obs_leave_graphics();
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

  cv::Mat imageBGRA;
  {
    std::unique_lock<std::mutex> lock(tf->inputBGRALock, std::try_to_lock);
    if (!lock.owns_lock()) {
      return;
    }
    imageBGRA = tf->inputBGRA.clone();
  }

  cv::Mat outputImage;
  if (!runFilterModelInference(tf, imageBGRA, outputImage)) {
    return;
  }

  {
    std::unique_lock<std::mutex> lock(tf->outputLock, std::try_to_lock);
    if (!lock.owns_lock()) {
      return;
    }
    cv::cvtColor(outputImage, tf->outputBGRA, cv::COLOR_BGR2RGBA);
  }
}

static void filter_video_render(void *data, gs_effect_t *_effect)
{
  UNUSED_PARAMETER(_effect);

  struct enhance_filter *tf = reinterpret_cast<enhance_filter *>(data);
  uint32_t width, height;
  if (!getRGBAFromStageSurface(tf, width, height)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }

  if (!obs_source_process_filter_begin(tf->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }
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

  gs_blend_state_push();
  gs_reset_blend_state();

  gs_effect_t* effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
  while (gs_effect_loop(effect, "Draw")) {
    obs_source_draw(outputTexture, 0, 0, width, height, false);
  }

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
