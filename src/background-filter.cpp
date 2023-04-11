#include <obs-module.h>

#include <onnxruntime_cxx_api.h>
#include <cpu_provider_factory.h>

#if defined(__APPLE__)
#include <coreml_provider_factory.h>
#endif

#ifdef WITH_CUDA
#include <cuda_provider_factory.h>
#endif

#ifdef _WIN32
#ifndef WITH_CUDA
#include <dml_provider_factory.h>
#endif
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

const char *MODEL_SINET = "models/SINet_Softmax_simple.onnx";
const char *MODEL_MEDIAPIPE = "models/mediapipe.onnx";
const char *MODEL_SELFIE = "models/selfie_segmentation.onnx";
const char *MODEL_RVM = "models/rvm_mobilenetv3_fp32.onnx";
const char *MODEL_PPHUMANSEG = "models/pphumanseg_fp32.onnx";

const char *USEGPU_CPU = "cpu";
const char *USEGPU_DML = "dml";
const char *USEGPU_CUDA = "cuda";
const char *USEGPU_COREML = "coreml";

struct background_removal_filter {
  std::unique_ptr<Ort::Session> session;
  std::unique_ptr<Ort::Env> env;
  std::vector<Ort::AllocatedStringPtr> inputNames;
  std::vector<Ort::AllocatedStringPtr> outputNames;
  std::vector<Ort::Value> inputTensor;
  std::vector<Ort::Value> outputTensor;
  std::vector<std::vector<int64_t>> inputDims;
  std::vector<std::vector<int64_t>> outputDims;
  std::vector<std::vector<float>> outputTensorValues;
  std::vector<std::vector<float>> inputTensorValues;
  float threshold = 0.5f;
  cv::Scalar backgroundColor{0, 0, 0, 0};
  float contourFilter = 0.05f;
  float smoothContour = 0.5f;
  float feather = 0.0f;
  std::string useGPU;
  std::string modelSelection;
  std::unique_ptr<Model> model;

  obs_source_t *source;
  gs_texrender_t *texrender;
  gs_stagesurf_t *stagesurface;

  cv::Mat backgroundMask;
  int maskEveryXFrames = 1;
  int maskEveryXFramesCount = 0;
  int64_t blurBackground = 0;

  cv::Mat inputBGRA;
  cv::Mat outputBGRA;

  bool isDisabled;

  std::mutex inputBGRALock;
  std::mutex outputBGRALock;

#if _WIN32
  const wchar_t *modelFilepath = nullptr;
#else
  const char *modelFilepath = nullptr;
#endif
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
#ifdef WITH_CUDA
  obs_property_list_add_string(p_use_gpu, obs_module_text("GPUCUDA"), USEGPU_CUDA);
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
                                obs_module_text("BlurBackgroundFactor0NoBlurUseColor"), 0, 100, 1);

  UNUSED_PARAMETER(data);
  return props;
}

static void filter_defaults(obs_data_t *settings)
{
  obs_data_set_default_double(settings, "threshold", 0.5);
  obs_data_set_default_double(settings, "contour_filter", 0.05);
  obs_data_set_default_double(settings, "smooth_contour", 0.5);
  obs_data_set_default_double(settings, "feather", 0.0);
  obs_data_set_default_string(settings, "useGPU", USEGPU_CPU);
  obs_data_set_default_string(settings, "model_select", MODEL_MEDIAPIPE);
  obs_data_set_default_int(settings, "mask_every_x_frames", 1);
}

static void createOrtSession(struct background_removal_filter *tf)
{
  if (tf->model.get() == nullptr) {
    blog(LOG_ERROR, "Model object is not initialized");
    return;
  }

  Ort::SessionOptions sessionOptions;

  sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
  if (tf->useGPU != USEGPU_CPU) {
    sessionOptions.DisableMemPattern();
    sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
  }

  char *modelFilepath_rawPtr = obs_module_file(tf->modelSelection.c_str());

  if (modelFilepath_rawPtr == nullptr) {
    blog(LOG_ERROR, "Unable to get model filename %s from plugin.", tf->modelSelection.c_str());
    return;
  }

  std::string modelFilepath_s(modelFilepath_rawPtr);
  bfree(modelFilepath_rawPtr);

#if _WIN32
  std::wstring modelFilepath_ws(modelFilepath_s.size(), L' ');
  std::copy(modelFilepath_s.begin(), modelFilepath_s.end(), modelFilepath_ws.begin());
  tf->modelFilepath = modelFilepath_ws.c_str();
#else
  tf->modelFilepath = modelFilepath_s.c_str();
#endif

  try {
#ifdef WITH_CUDA
    if (tf->useGPU == USEGPU_CUDA) {
      Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(sessionOptions, 0));
    }
#endif
#ifdef _WIN32
    if (tf->useGPU == USEGPU_DML) {
      Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0));
    }
#endif
#if defined(__APPLE__)
    if (tf->useGPU == USEGPU_COREML) {
      uint32_t coreml_flags = 0;
      coreml_flags |= COREML_FLAG_ENABLE_ON_SUBGRAPH;
      Ort::ThrowOnError(
        OrtSessionOptionsAppendExecutionProvider_CoreML(sessionOptions, coreml_flags));
    }
#endif
    tf->session.reset(new Ort::Session(*tf->env, tf->modelFilepath, sessionOptions));
  } catch (const std::exception &e) {
    blog(LOG_ERROR, "%s", e.what());
    return;
  }

  Ort::AllocatorWithDefaultOptions allocator;

  tf->model->populateInputOutputNames(tf->session, tf->inputNames, tf->outputNames);

  if (!tf->model->populateInputOutputShapes(tf->session, tf->inputDims, tf->outputDims)) {
    blog(LOG_ERROR, "Unable to get model input and output shapes");
    return;
  }

  for (size_t i = 0; i < tf->inputNames.size(); i++) {
    blog(LOG_INFO, "Model %s input %d: name %s shape (%d dim) %d x %d x %d x %d",
         tf->modelSelection.c_str(), (int)i, tf->inputNames[i].get(), (int)tf->inputDims[i].size(),
         (int)tf->inputDims[i][0],
         ((int)tf->inputDims[i].size() > 1) ? (int)tf->inputDims[i][1] : 0,
         ((int)tf->inputDims[i].size() > 2) ? (int)tf->inputDims[i][2] : 0,
         ((int)tf->inputDims[i].size() > 3) ? (int)tf->inputDims[i][3] : 0);
  }
  for (size_t i = 0; i < tf->outputNames.size(); i++) {
    blog(LOG_INFO, "Model %s output %d: name %s shape (%d dim) %d x %d x %d x %d",
         tf->modelSelection.c_str(), (int)i, tf->outputNames[i].get(),
         (int)tf->outputDims[i].size(), (int)tf->outputDims[i][0],
         ((int)tf->outputDims[i].size() > 1) ? (int)tf->outputDims[i][1] : 0,
         ((int)tf->outputDims[i].size() > 2) ? (int)tf->outputDims[i][2] : 0,
         ((int)tf->outputDims[i].size() > 3) ? (int)tf->outputDims[i][3] : 0);
  }

  // Allocate buffers
  tf->model->allocateTensorBuffers(tf->inputDims, tf->outputDims, tf->outputTensorValues,
                                   tf->inputTensorValues, tf->inputTensor, tf->outputTensor);
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

  if (tf->modelSelection.empty() || tf->modelSelection != newModel || tf->useGPU != newUseGpu) {
    // Re-initialize model if it's not already the selected one or switching inference device
    tf->modelSelection = newModel;
    tf->useGPU = newUseGpu;

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
    obs_leave_graphics();
    tf->~background_removal_filter();
    bfree(tf);
  }
}

static void processImageForBackground(struct background_removal_filter *tf,
                                      const cv::Mat &imageBGRA, cv::Mat &backgroundMask)
{
  if (tf->session.get() == nullptr) {
    // Onnx runtime session is not initialized. Problem in initialization
    return;
  }
  try {
    // To RGB
    cv::Mat imageRGB;
    cv::cvtColor(imageBGRA, imageRGB, cv::COLOR_BGRA2RGB);

    // Resize to network input size
    uint32_t inputWidth, inputHeight;
    tf->model->getNetworkInputSize(tf->inputDims, inputWidth, inputHeight);

    cv::Mat resizedImageRGB;
    cv::resize(imageRGB, resizedImageRGB, cv::Size(inputWidth, inputHeight));

    // Prepare input to nework
    cv::Mat resizedImage, preprocessedImage;
    resizedImageRGB.convertTo(resizedImage, CV_32F);

    tf->model->prepareInputToNetwork(resizedImage, preprocessedImage);

    tf->model->loadInputToTensor(preprocessedImage, inputWidth, inputHeight, tf->inputTensorValues);

    // Run network inference
    tf->model->runNetworkInference(tf->session, tf->inputNames, tf->outputNames, tf->inputTensor,
                                   tf->outputTensor);

    // Get output
    // Map network output mask to cv::Mat
    cv::Mat outputImage = tf->model->getNetworkOutput(tf->outputDims, tf->outputTensorValues);

    // Assign output to input in some models that have temporal information
    tf->model->assignOutputToInput(tf->outputTensorValues, tf->inputTensorValues);

    // Post-process output
    tf->model->postprocessOutput(outputImage);

    if (tf->modelSelection == MODEL_SINET || tf->modelSelection == MODEL_MEDIAPIPE) {
      backgroundMask = outputImage > tf->threshold;
    } else {
      backgroundMask = outputImage < tf->threshold;
    }

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

    // Smooth mask with a fast filter on the small mask before the final resize
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
  } catch (const std::exception &e) {
    blog(LOG_ERROR, "%s", e.what());
  }
}

// Blend
void blend_images_with_mask(cv::Mat &dst, const cv::Mat &src, const cv::Mat &mask)
{
  for (size_t i = 0; i < dst.total(); i++) {
    const auto maskPixel = mask.at<uchar>((int)i);
    if (maskPixel == 0) {
      continue;
    }

    const auto &srcPixel = src.at<cv::Vec4b>((int)i);
    auto &dstPixel = dst.at<cv::Vec4b>((int)i);

    if (maskPixel == 255) {
      dstPixel = srcPixel;
    } else {
      const float alpha = maskPixel / 255.0f;
      dstPixel = dstPixel * (1.0f - alpha) + srcPixel * alpha;
    }
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
      // Process the image to find the mask.
      processImageForBackground(tf, imageBGRA, tf->backgroundMask);

      if (tf->feather > 0.0) {
        // Feather (blur) the mask
        const int k_size = (int)(40 * tf->feather);
        cv::dilate(tf->backgroundMask, tf->backgroundMask, cv::Mat(), cv::Point(-1, -1),
                   k_size / 3);
        cv::boxFilter(tf->backgroundMask, tf->backgroundMask, tf->backgroundMask.depth(),
                      cv::Size(k_size, k_size));
      }
    }

    // Apply the mask back to the main image.
    if (tf->blurBackground > 0.0) {
      // Blur the background (fast box filter)
      int k_size = (int)(5 + tf->blurBackground);
      k_size += k_size % 2 == 0 ? 1 : 0;

      cv::Mat blurredBackground;
      cv::stackBlur(imageBGRA, blurredBackground, cv::Size(k_size, k_size));

      // Blend the blurred background with the main image
      blend_images_with_mask(imageBGRA, blurredBackground, tf->backgroundMask);
    } else {
      // Set the main image to the background color where the mask is 0
      cv::Mat inverseMask = cv::Scalar(255) - tf->backgroundMask;
      int from_to[] = {0, 3}; // alpha[0] -> bgra[3]
      cv::mixChannels(&inverseMask, 1, &imageBGRA, 1, from_to, 1);
    }
  } catch (const std::exception &e) {
    blog(LOG_ERROR, "%s", e.what());
  }

  {
    std::unique_lock<std::mutex> lock(tf->inputBGRALock, std::try_to_lock);
    if (!lock.owns_lock()) {
      return;
    }
    tf->outputBGRA = imageBGRA.clone();
  }

  UNUSED_PARAMETER(seconds);
}

static void filter_video_render(void *data, gs_effect_t *_effect)
{
  struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

  if (!obs_source_enabled(tf->source)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }

  obs_source_t *target = obs_filter_get_target(tf->source);
  if (!target) {
    obs_source_skip_video_filter(tf->source);
    return;
  }
  const uint32_t width = obs_source_get_base_width(target);
  const uint32_t height = obs_source_get_base_height(target);
  if (width == 0 || height == 0) {
    obs_source_skip_video_filter(tf->source);
    return;
  }
  gs_texrender_reset(tf->texrender);
  if (!gs_texrender_begin(tf->texrender, width, height)) {
    obs_source_skip_video_filter(tf->source);
    return;
  }
  struct vec4 background;
  vec4_zero(&background);
  gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
  gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
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
    obs_source_skip_video_filter(tf->source);
    return;
  }
  {
    std::lock_guard<std::mutex> lock(tf->inputBGRALock);
    tf->inputBGRA = cv::Mat(height, width, CV_8UC4, video_data, linesize);
  }
  gs_stagesurface_unmap(tf->stagesurface);

  if (tf->outputBGRA.empty() || static_cast<uint32_t>(tf->outputBGRA.cols) != width ||
      static_cast<uint32_t>(tf->outputBGRA.rows) != height) {
    obs_source_skip_video_filter(tf->source);
    return;
  }
  cv::Mat imageBGRA;
  {
    std::lock_guard<std::mutex> lock(tf->outputBGRALock);
    imageBGRA = tf->outputBGRA.clone();
  }
  const uint8_t *textureData[] = {imageBGRA.data};
  gs_texture_t *texture = gs_texture_create(width, height, GS_BGRA, 1, textureData, 0);
  gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
  gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
  gs_effect_set_texture(image, texture);
  gs_blend_state_push();
  gs_reset_blend_state();
  while (gs_effect_loop(effect, "Draw")) {
    gs_draw_sprite(texture, 0, width, height);
  }
  gs_blend_state_pop();
  gs_texture_destroy(texture);

  UNUSED_PARAMETER(_effect);
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
