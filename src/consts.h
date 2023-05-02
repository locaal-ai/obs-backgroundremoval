#ifndef CONSTS_H
#define CONSTS_H

const char * const MODEL_SINET = "models/SINet_Softmax_simple.onnx";
const char * const MODEL_MEDIAPIPE = "models/mediapipe.onnx";
const char * const MODEL_SELFIE = "models/selfie_segmentation.onnx";
const char * const MODEL_RVM = "models/rvm_mobilenetv3_fp32.onnx";
const char * const MODEL_PPHUMANSEG = "models/pphumanseg_fp32.onnx";
const char * const MODEL_ENHANCE = "models/tbefn_fp32.onnx";

const char * const USEGPU_CPU = "cpu";
const char * const USEGPU_DML = "dml";
const char * const USEGPU_CUDA = "cuda";
const char * const USEGPU_TENSORRT = "tensorrt";
const char * const USEGPU_COREML = "coreml";

const char * const EFFECT_PATH = "effects/mask_alpha_filter.effect";
const char * const KAWASE_BLUR_EFFECT_PATH = "effects/kawase_blur.effect";
const char * const BLEND_EFFECT_PATH = "effects/blend_images.effect";

#endif /* CONSTS_H */
