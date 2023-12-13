#ifndef CONSTS_H
#define CONSTS_H

const char *const MODEL_SINET = "models/SINet_Softmax_simple.onnx";
const char *const MODEL_MEDIAPIPE = "models/mediapipe.onnx";
const char *const MODEL_SELFIE = "models/selfie_segmentation.onnx";
const char *const MODEL_RVM = "models/rvm_mobilenetv3_fp32.onnx";
const char *const MODEL_PPHUMANSEG = "models/pphumanseg_fp32.onnx";
const char *const MODEL_ENHANCE_TBEFN = "models/tbefn_fp32.onnx";
const char *const MODEL_ENHANCE_URETINEX = "models/uretinex_net_180x320.onnx";
const char *const MODEL_ENHANCE_SGLLIE =
	"models/semantic_guided_llie_180x324.onnx";
const char *const MODEL_ENHANCE_ZERODCE = "models/zero_dce_180x320.onnx";
const char *const MODEL_DEPTH_TCMONODEPTH =
	"models/tcmonodepth_tcsmallnet_192x320.onnx";

const char *const USEGPU_CPU = "cpu";
const char *const USEGPU_DML = "dml";
const char *const USEGPU_CUDA = "cuda";
const char *const USEGPU_TENSORRT = "tensorrt";
const char *const USEGPU_COREML = "coreml";

const char *const EFFECT_PATH = "effects/mask_alpha_filter.effect";
const char *const KAWASE_BLUR_EFFECT_PATH = "effects/kawase_blur.effect";
const char *const BLEND_EFFECT_PATH = "effects/blend_images.effect";

const char *const PLUGIN_INFO_TEMPLATE =
	"<a href=\"https://github.com/occ-ai/obs-backgroundremoval/\">Background Removal</a> (%1) by <a href=\"https://github.com/occ-ai\">OCC AI</a>";

#endif /* CONSTS_H */
