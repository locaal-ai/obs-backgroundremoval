#include <obs-module.h>

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

enum YUV_TYPE {
	YUV_TYPE_UYVY = 0,
	YUV_TYPE_YVYU = 1,
	YUV_TYPE_YUYV = 2,
};

void rgb_to_yuv(const cv::Mat& rgb, cv::Mat& yuv, YUV_TYPE yuvType = YUV_TYPE_UYVY) {
	assert(rgb.depth() == CV_8U &&
		   rgb.channels() == 3 &&
		   yuv.depth() == CV_8U &&
		   yuv.channels() == 2 &&
		   rgb.size() == yuv.size());

	for (int ih = 0; ih < rgb.rows; ih++) {
		const uint8_t* rgbRowPtr = rgb.ptr<uint8_t>(ih);
		uint8_t* yuvRowPtr = yuv.ptr<uint8_t>(ih);

		for (int iw = 0; iw < rgb.cols; iw = iw + 2) {
			const int rgbColIdxBytes = iw * rgb.elemSize();
			const int yuvColIdxBytes = iw * yuv.elemSize();

			const uint8_t R1 = rgbRowPtr[rgbColIdxBytes + 0];
			const uint8_t G1 = rgbRowPtr[rgbColIdxBytes + 1];
			const uint8_t B1 = rgbRowPtr[rgbColIdxBytes + 2];
			const uint8_t R2 = rgbRowPtr[rgbColIdxBytes + 3];
			const uint8_t G2 = rgbRowPtr[rgbColIdxBytes + 4];
			const uint8_t B2 = rgbRowPtr[rgbColIdxBytes + 5];

			const int Y  =  (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16.0f ;
			const int U  = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128.0f;
			const int V  =  (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128.0f;
			const int Y2 =  (0.257f * R2) + (0.504f * G2) + (0.098f * B2) + 16.0f ;

			if (yuvType == YUV_TYPE_UYVY) {
				yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(U );
				yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(Y );
				yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(V );
				yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(Y2);
			}
			if (yuvType == YUV_TYPE_YVYU) {
				yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(Y );
				yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(V );
				yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(Y2);
				yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(U );
			}
			if (yuvType == YUV_TYPE_YUYV) {
				yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(Y );
				yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(U );
				yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(Y2);
				yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(V );
			}
		}
	}
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

	std::string instanceName{"background-removal-inference"};
	char* modelFilepath_rawPtr = obs_module_file("SINet_Softmax.onnx");
#if _WIN32
	std::string modelFilepath_s(modelFilepath_rawPtr);
    std::wstring modelFilepath_ws(modelFilepath_s.size(), L' ');
    std::copy(modelFilepath_s.begin(), modelFilepath_s.end(), modelFilepath_ws.begin());
    const wchar_t* modelFilepath = modelFilepath_ws.c_str();
#else
    const char* modelFilepath = std::string(modelFilepath_rawPtr).c_str();
#endif
	bfree(modelFilepath_rawPtr);
	blog(LOG_INFO, "model location %s", modelFilepath);

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

cv::Mat convertFrameToRGB(struct obs_source_frame *frame) {
	cv::Mat imageRGB;
	if (frame->format == VIDEO_FORMAT_UYVY) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_UYVY);
	}
	if (frame->format == VIDEO_FORMAT_YVYU) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_YVYU);
	}
	if (frame->format == VIDEO_FORMAT_YUY2) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_YUY2);
	}
	if (frame->format == VIDEO_FORMAT_NV12) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_NV12);
	}
	if (frame->format == VIDEO_FORMAT_I420) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_I420);
	}
	if (frame->format == VIDEO_FORMAT_I444) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC3, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB);
	}
	if (frame->format == VIDEO_FORMAT_RGBA) {
		cv::Mat imageRGBA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageRGBA, imageRGB, cv::ColorConversionCodes::COLOR_RGBA2RGB);
	}
	if (frame->format == VIDEO_FORMAT_BGRA) {
		cv::Mat imageBGRA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageBGRA, imageRGB, cv::ColorConversionCodes::COLOR_BGRA2RGB);
	}
	if (frame->format == VIDEO_FORMAT_Y800) {
		cv::Mat imageGray(frame->height, frame->width, CV_8UC1, frame->data[0]);
		cv::cvtColor(imageGray, imageRGB, cv::ColorConversionCodes::COLOR_GRAY2RGB);
	}
	return imageRGB;
}

void convertRGBToFrame(cv::Mat imageRGB, struct obs_source_frame *frame) {
	if (frame->format == VIDEO_FORMAT_UYVY) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		rgb_to_yuv(imageRGB, imageYUV, YUV_TYPE_UYVY);
	}
	if (frame->format == VIDEO_FORMAT_YVYU) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		rgb_to_yuv(imageRGB, imageYUV, YUV_TYPE_YVYU);
	}
	if (frame->format == VIDEO_FORMAT_YUY2) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		rgb_to_yuv(imageRGB, imageYUV, YUV_TYPE_YUYV);
	}
	if (frame->format == VIDEO_FORMAT_NV12) {
		cv::Mat imageYUV(frame->height, frame->width * 3 / 2, CV_8UC1, frame->data[0]);
		cv::cvtColor(imageRGB, imageYUV, cv::ColorConversionCodes::COLOR_RGB2YUV_YV12);
	}
	if (frame->format == VIDEO_FORMAT_I420) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageRGB, imageYUV, cv::ColorConversionCodes::COLOR_RGB2YUV_I420);
	}
	if (frame->format == VIDEO_FORMAT_I444) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC3, frame->data[0]);
		cv::cvtColor(imageRGB, imageYUV, cv::ColorConversionCodes::COLOR_RGB2YUV);
	}
	if (frame->format == VIDEO_FORMAT_RGBA) {
		cv::Mat imageRGBA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageRGB, imageRGBA, cv::ColorConversionCodes::COLOR_RGB2RGBA);
	}
	if (frame->format == VIDEO_FORMAT_BGRA) {
		cv::Mat imageBGRA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageRGB, imageBGRA, cv::ColorConversionCodes::COLOR_RGB2BGRA);
	}
	if (frame->format == VIDEO_FORMAT_Y800) {
		cv::Mat imageGray(frame->height, frame->width, CV_8UC1, frame->data[0]);
		cv::cvtColor(imageRGB, imageGray, cv::ColorConversionCodes::COLOR_RGB2GRAY);
	}
}

static struct obs_source_frame * filter_render(void *data, struct obs_source_frame *frame)
{
	struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

	// Convert to RGB
	cv::Mat imageRGB = convertFrameToRGB(frame);

	// Prepare input to nework
    cv::Mat resizedImageRGB, resizedImage, preprocessedImage;
    cv::resize(imageRGB, resizedImageRGB,
               cv::Size(tf->inputDims.at(2), tf->inputDims.at(3)),
               cv::InterpolationFlags::INTER_CUBIC);
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
	cv::Mat outputImageReized;
	cv::resize(outputImage, outputImageReized, imageRGB.size(), cv::InterpolationFlags::INTER_CUBIC);

	cv::Mat mask = outputImageReized > tf->threshold;

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
	imageRGB.setTo(tf->backgroundColor, mask);

	// Put masked image back on frame
	convertRGBToFrame(imageRGB, frame);

	return frame;
}

static void filter_destroy(void *data)
{
	struct background_removal_filter *tf = reinterpret_cast<background_removal_filter *>(data);

	if (tf) {
		bfree(tf);
	}
}



struct obs_source_info background_removal_filter_info = {
	.id = "background_removal_filter",
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
