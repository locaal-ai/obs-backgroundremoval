#include <onnxruntime_cxx_api.h>
#include <cpu_provider_factory.h>

#if defined(__APPLE__)
#include <coreml_provider_factory.h>
#endif

#if defined(__linux__) && defined(__x86_64__)
#include <tensorrt_provider_factory.h>
#endif

#ifdef _WIN32
#include <dml_provider_factory.h>
#include <wchar.h>
#endif // _WIN32

#include <obs-module.h>

#include "ort-session-utils.h"
#include "consts.h"
#include "plugin-support.h"

int createOrtSession(filter_data *tf)
{
	if (tf->model.get() == nullptr) {
		obs_log(LOG_ERROR, "Model object is not initialized");
		return OBS_BGREMOVAL_ORT_SESSION_ERROR_INVALID_MODEL;
	}

	Ort::SessionOptions sessionOptions;

	sessionOptions.SetGraphOptimizationLevel(
		GraphOptimizationLevel::ORT_ENABLE_ALL);
	if (tf->useGPU != USEGPU_CPU) {
		sessionOptions.DisableMemPattern();
		sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
	} else {
		sessionOptions.SetInterOpNumThreads(tf->numThreads);
		sessionOptions.SetIntraOpNumThreads(tf->numThreads);
	}

	char *modelFilepath_rawPtr =
		obs_module_file(tf->modelSelection.c_str());

	if (modelFilepath_rawPtr == nullptr) {
		obs_log(LOG_ERROR,
			"Unable to get model filename %s from plugin.",
			tf->modelSelection.c_str());
		return OBS_BGREMOVAL_ORT_SESSION_ERROR_FILE_NOT_FOUND;
	}

	std::string modelFilepath_s(modelFilepath_rawPtr);
	bfree(modelFilepath_rawPtr);

#if _WIN32
	std::wstring modelFilepath_ws(modelFilepath_s.size(), L' ');
	std::copy(modelFilepath_s.begin(), modelFilepath_s.end(),
		  modelFilepath_ws.begin());
	tf->modelFilepath = modelFilepath_ws.c_str();
#else
	tf->modelFilepath = modelFilepath_s.c_str();
#endif

	try {
#if defined(__linux__) && defined(__x86_64__)
		if (tf->useGPU == USEGPU_TENSORRT) {
			Ort::ThrowOnError(
				OrtSessionOptionsAppendExecutionProvider_Tensorrt(
					sessionOptions, 0));
		}
#endif
#ifdef _WIN32
		if (tf->useGPU == USEGPU_DML) {
			auto &api = Ort::GetApi();
			OrtDmlApi *dmlApi = nullptr;
			Ort::ThrowOnError(api.GetExecutionProviderApi(
				"DML", ORT_API_VERSION,
				(const void **)&dmlApi));
			Ort::ThrowOnError(
				dmlApi->SessionOptionsAppendExecutionProvider_DML(
					sessionOptions, 0));
		}
#endif
#if defined(__APPLE__)
		if (tf->useGPU == USEGPU_COREML) {
			uint32_t coreml_flags = 0;
			coreml_flags |= COREML_FLAG_ENABLE_ON_SUBGRAPH;
			Ort::ThrowOnError(
				OrtSessionOptionsAppendExecutionProvider_CoreML(
					sessionOptions, coreml_flags));
		}
#endif
		tf->session.reset(new Ort::Session(*tf->env, tf->modelFilepath,
						   sessionOptions));
	} catch (const std::exception &e) {
		obs_log(LOG_ERROR, "%s", e.what());
		return OBS_BGREMOVAL_ORT_SESSION_ERROR_STARTUP;
	}

	Ort::AllocatorWithDefaultOptions allocator;

	tf->model->populateInputOutputNames(tf->session, tf->inputNames,
					    tf->outputNames);

	if (!tf->model->populateInputOutputShapes(tf->session, tf->inputDims,
						  tf->outputDims)) {
		obs_log(LOG_ERROR,
			"Unable to get model input and output shapes");
		return OBS_BGREMOVAL_ORT_SESSION_ERROR_INVALID_INPUT_OUTPUT;
	}

	for (size_t i = 0; i < tf->inputNames.size(); i++) {
		obs_log(LOG_INFO,
			"Model %s input %d: name %s shape (%d dim) %d x %d x %d x %d",
			tf->modelSelection.c_str(), (int)i,
			tf->inputNames[i].get(), (int)tf->inputDims[i].size(),
			(int)tf->inputDims[i][0],
			((int)tf->inputDims[i].size() > 1)
				? (int)tf->inputDims[i][1]
				: 0,
			((int)tf->inputDims[i].size() > 2)
				? (int)tf->inputDims[i][2]
				: 0,
			((int)tf->inputDims[i].size() > 3)
				? (int)tf->inputDims[i][3]
				: 0);
	}
	for (size_t i = 0; i < tf->outputNames.size(); i++) {
		obs_log(LOG_INFO,
			"Model %s output %d: name %s shape (%d dim) %d x %d x %d x %d",
			tf->modelSelection.c_str(), (int)i,
			tf->outputNames[i].get(), (int)tf->outputDims[i].size(),
			(int)tf->outputDims[i][0],
			((int)tf->outputDims[i].size() > 1)
				? (int)tf->outputDims[i][1]
				: 0,
			((int)tf->outputDims[i].size() > 2)
				? (int)tf->outputDims[i][2]
				: 0,
			((int)tf->outputDims[i].size() > 3)
				? (int)tf->outputDims[i][3]
				: 0);
	}

	// Allocate buffers
	tf->model->allocateTensorBuffers(tf->inputDims, tf->outputDims,
					 tf->outputTensorValues,
					 tf->inputTensorValues, tf->inputTensor,
					 tf->outputTensor);

	return OBS_BGREMOVAL_ORT_SESSION_SUCCESS;
}

bool runFilterModelInference(filter_data *tf, const cv::Mat &imageBGRA,
			     cv::Mat &output)
{
	if (tf->session.get() == nullptr) {
		// Onnx runtime session is not initialized. Problem in initialization
		return false;
	}
	if (tf->model.get() == nullptr) {
		// Model object is not initialized
		return false;
	}

	// To RGB
	cv::Mat imageRGB;
	cv::cvtColor(imageBGRA, imageRGB, cv::COLOR_BGRA2RGB);

	// Resize to network input size
	uint32_t inputWidth, inputHeight;
	tf->model->getNetworkInputSize(tf->inputDims, inputWidth, inputHeight);

	cv::Mat resizedImageRGB;
	cv::resize(imageRGB, resizedImageRGB,
		   cv::Size(inputWidth, inputHeight));

	// Prepare input to nework
	cv::Mat resizedImage, preprocessedImage;
	resizedImageRGB.convertTo(resizedImage, CV_32F);

	tf->model->prepareInputToNetwork(resizedImage, preprocessedImage);

	tf->model->loadInputToTensor(preprocessedImage, inputWidth, inputHeight,
				     tf->inputTensorValues);

	// Run network inference
	tf->model->runNetworkInference(tf->session, tf->inputNames,
				       tf->outputNames, tf->inputTensor,
				       tf->outputTensor);

	// Get output
	// Map network output to cv::Mat
	cv::Mat outputImage = tf->model->getNetworkOutput(
		tf->outputDims, tf->outputTensorValues);

	// Assign output to input in some models that have temporal information
	tf->model->assignOutputToInput(tf->outputTensorValues,
				       tf->inputTensorValues);

	// Post-process output. The image will now be in [0,1] float, BHWC format
	tf->model->postprocessOutput(outputImage);

	// Convert [0,1] float to CV_8U [0,255]
	outputImage.convertTo(output, CV_8U, 255.0);

	return true;
}
