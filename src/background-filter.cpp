#include <obs-module.h>
#include <media-io/video-scaler.h>

#if defined(__APPLE__)
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <onnxruntime/core/providers/cpu/cpu_provider_factory.h>
#elif defined(__linux__) || defined(_WIN32)
#include <onnxruntime_cxx_api.h>
#include <cpu_provider_factory.h>
#endif
#if defined(_WIN32)
#include <wchar.h>
#endif

#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>

#include "plugin-macros.generated.h"

struct background_removal_filter {
	std::unique_ptr<Ort::Session> session;
	std::unique_ptr<Ort::Env> env;
	std::vector<const char*> inputNames;
	std::vector<const char*> outputNames;
	Ort::Value inputTensor;
	Ort::Value outputTensor;
	std::vector<int64_t> inputDims;
	std::vector<int64_t> outputDims;
	std::vector<float> outputTensorValues;
	std::vector<float> inputTensorValues;
	Ort::MemoryInfo memoryInfo;
	float threshold = 0.5;
	cv::Scalar backgroundColor{0, 0, 0};
	float contourFilter = 0.05;
	float smoothContour = 0.5;

	// Use the media-io converter to both scale and convert the colorspace
	video_scaler_t* scalerToRGB;
	video_scaler_t* scalerFromRGB;
};

static const char *filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Background Removal";
}

template <typename T>
T vectorProduct(const std::vector<T>& v)
{
    return accumulate(v.begin(), v.end(), 1, std::multiplies<T>());
}

void hwc_to_chw(cv::InputArray src, cv::OutputArray dst) {
    std::vector<cv::Mat> channels;
    cv::split(src, channels);

    // Stretch one-channel images to vector
    for (auto &img : channels) {
        img = img.reshape(1, 1);
    }

    // Concatenate three vectors to one
    cv::hconcat( channels, dst );
}


/**                   PROPERTIES                     */

static obs_properties_t *filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *p_threshold = obs_properties_add_float_slider(
		props,
		"threshold",
		obs_module_text("Threshold"),
		0.0,
		1.0,
		0.05);

	obs_property_t *p_contour_filter = obs_properties_add_float_slider(
		props,
		"contour_filter",
		obs_module_text("Contour Filter (% of image)"),
		0.0,
		1.0,
		0.025);

	obs_property_t *p_smooth_contour = obs_properties_add_float_slider(
		props,
		"smooth_contour",
		obs_module_text("Smooth silhouette"),
		0.0,
		1.0,
		0.05);

	obs_property_t *p_color = obs_properties_add_color(
		props,
		"replaceColor",
		obs_module_text("Background Color"));


	UNUSED_PARAMETER(data);
	return props;
}

static void filter_defaults(obs_data_t *settings) {
	obs_data_set_default_double(settings, "threshold", 0.5);
	obs_data_set_default_double(settings, "contour_filter", 0.05);
	obs_data_set_default_double(settings, "smooth_contour", 0.5);
	obs_data_set_default_int(settings, "replaceColor", 0x000000);
}

static void filter_update(void *data, obs_data_t *settings)
{
	struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);
	tf->threshold = obs_data_get_double(settings, "threshold");

	uint64_t color = obs_data_get_int(settings, "replaceColor");
	tf->backgroundColor.val[0] = color & 0x0000ff;
	tf->backgroundColor.val[1] = (color >> 8) & 0x0000ff;
	tf->backgroundColor.val[2] = (color >> 16) & 0x0000ff;

	tf->contourFilter = obs_data_get_double(settings, "contour_filter");
	tf->smoothContour = obs_data_get_double(settings, "smooth_contour");
}


/**                   FILTER CORE                     */

static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(bzalloc(sizeof(struct background_removal_filter)));

	char* modelFilepath_rawPtr = obs_module_file("SINet_Softmax.onnx");
	blog(LOG_INFO, "model location %s", modelFilepath_rawPtr);

	std::string modelFilepath_s(modelFilepath_rawPtr);
    bfree(modelFilepath_rawPtr);

#if _WIN32
    std::wstring modelFilepath_ws(modelFilepath_s.size(), L' ');
    std::copy(modelFilepath_s.begin(), modelFilepath_s.end(), modelFilepath_ws.begin());
    const wchar_t* modelFilepath = modelFilepath_ws.c_str();
#else
    const char* modelFilepath = modelFilepath_s.c_str();
#endif

	std::string instanceName{"background-removal-inference"};
    tf->env.reset(new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, instanceName.c_str()));
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    tf->session.reset(new Ort::Session(*tf->env, modelFilepath, sessionOptions));

    Ort::AllocatorWithDefaultOptions allocator;

    tf->inputNames.push_back(tf->session->GetInputName(0, allocator));
    tf->outputNames.push_back(tf->session->GetOutputName(0, allocator));

	// Allocate output buffer
    const Ort::TypeInfo outputTypeInfo = tf->session->GetOutputTypeInfo(0);
    const auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
    tf->outputDims = outputTensorInfo.GetShape();
    tf->outputTensorValues.resize(vectorProduct(tf->outputDims));

	// Allocate input buffer
    const Ort::TypeInfo inputTypeInfo = tf->session->GetInputTypeInfo(0);
    const auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
    tf->inputDims = inputTensorInfo.GetShape();
    tf->inputTensorValues.resize(vectorProduct(tf->inputDims));

	// Build input and output tensors
    tf->memoryInfo = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU);
    tf->outputTensor = Ort::Value::CreateTensor<float>(
        	tf->memoryInfo,
			tf->outputTensorValues.data(),
			tf->outputTensorValues.size(),
        	tf->outputDims.data(),
			tf->outputDims.size());
	tf->inputTensor = Ort::Value::CreateTensor<float>(
			tf->memoryInfo,
			tf->inputTensorValues.data(),
			tf->inputTensorValues.size(),
			tf->inputDims.data(),
			tf->inputDims.size());

	filter_update(tf, settings);

	return tf;
}


void initializeScalers(cv::Size rgbSize,
					   cv::Size frameSize,
					   enum video_format frameFormat,
					   struct background_removal_filter *tf) {
	struct video_scale_info dst{
		VIDEO_FORMAT_BGR3,
		(uint32_t)rgbSize.width,
		(uint32_t)rgbSize.height,
		VIDEO_RANGE_DEFAULT,
		VIDEO_CS_DEFAULT
	};
	struct video_scale_info src{
		frameFormat,
		(uint32_t)frameSize.width,
		(uint32_t)frameSize.height,
		VIDEO_RANGE_DEFAULT,
		VIDEO_CS_DEFAULT
	};
	if (tf->scalerToRGB == nullptr) {
		video_scaler_create(&tf->scalerToRGB, &dst, &src, VIDEO_SCALE_DEFAULT);
	}
	if (tf->scalerFromRGB == nullptr) {
		video_scaler_create(&tf->scalerFromRGB, &src, &dst, VIDEO_SCALE_DEFAULT);
	}
}


cv::Mat convertFrameToRGB(struct obs_source_frame *frame,
						  cv::Size rgbSize,
						  struct background_removal_filter *tf) {

	if (tf->scalerToRGB == nullptr) {
		// Lazy initialize the frame scale & color converter
		initializeScalers(rgbSize, cv::Size(frame->width, frame->height), frame->format, tf);
	}

	cv::Mat imageRGB(rgbSize, CV_8UC3);
	const uint32_t rgbLinesize = imageRGB.cols * imageRGB.elemSize();
	video_scaler_scale(tf->scalerToRGB,
		&(imageRGB.data), &(rgbLinesize),
		frame->data, frame->linesize);

	return imageRGB;
}


void convertRGBToFrame(cv::Mat imageRGB, struct obs_source_frame *frame, struct background_removal_filter *tf) {
	if (tf->scalerFromRGB == nullptr) {
		// Lazy initialize the frame scale & color converter
		initializeScalers(imageRGB.size(), cv::Size(frame->width, frame->height), frame->format, tf);
	}

	const uint32_t rgbLinesize = imageRGB.cols * imageRGB.elemSize();
	video_scaler_scale(tf->scalerFromRGB,
		frame->data, frame->linesize,
		&(imageRGB.data), &(rgbLinesize));
}


static struct obs_source_frame * filter_render(void *data, struct obs_source_frame *frame)
{
	struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

	// Convert to RGB
	cv::Mat resizedImageRGB = convertFrameToRGB(frame, cv::Size(tf->inputDims.at(2), tf->inputDims.at(3)), tf);
	// Prepare input to nework
    cv::Mat resizedImage, preprocessedImage;
    resizedImageRGB.convertTo(resizedImage, CV_32F);
	cv::subtract(resizedImage, cv::Scalar(102.890434, 111.25247, 126.91212), resizedImage);
	cv::multiply(resizedImage, cv::Scalar(1.0 / 62.93292,  1.0 / 62.82138, 1.0 / 66.355705) / 255.0, resizedImage);

    hwc_to_chw(resizedImage, preprocessedImage);

    tf->inputTensorValues.assign(preprocessedImage.begin<float>(),
                             	 preprocessedImage.end<float>());

	// Run network inference
    tf->session->Run(
		Ort::RunOptions{nullptr},
		tf->inputNames.data(), &(tf->inputTensor), 1,
		tf->outputNames.data(), &(tf->outputTensor), 1);

	// Convert network output mask to input size
    cv::Mat outputImage(tf->outputDims.at(2), tf->outputDims.at(3), CV_32FC1, &(tf->outputTensorValues[0]));

	cv::Mat mask = outputImage > tf->threshold;

	// Contour processing
	if (tf->contourFilter > 0.0 && tf->contourFilter < 1.0) {
		std::vector<std::vector<cv::Point> > contours;
		findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
		std::vector<std::vector<cv::Point> > filteredContours;
		const int64_t contourSizeThreshold = (int64_t)(mask.total() * tf->contourFilter);
		for (auto& contour : contours) {
			if (cv::contourArea(contour) > contourSizeThreshold) {
				filteredContours.push_back(contour);
			}
		}
		mask.setTo(0);
		drawContours(mask, filteredContours, -1, cv::Scalar(255), -1);
	}

	// Smooth mask with a fast filter (box)
	if (tf->smoothContour > 0.0) {
		int k_size = 100 * tf->smoothContour;
		cv::boxFilter(mask, mask, mask.depth(), cv::Size(k_size, k_size));
		mask = mask > 128;
	}

	// Mask the input
	resizedImageRGB.setTo(tf->backgroundColor, mask);

	// Put masked image back on frame
	convertRGBToFrame(resizedImageRGB, frame, tf);
	return frame;
}


static void filter_destroy(void *data)
{
	struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

	if (tf) {
		video_scaler_destroy(tf->scalerToRGB);
		video_scaler_destroy(tf->scalerFromRGB);
		bfree(tf);
	}
}



struct obs_source_info background_removal_filter_info = {
	.id = "background_removal",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
	.get_name = filter_getname,
	.create = filter_create,
	.destroy = filter_destroy,
	.get_defaults = filter_defaults,
	.get_properties = filter_properties,
	.update = filter_update,
	.filter_video = filter_render,
};
