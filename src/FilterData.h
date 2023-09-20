#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs-module.h>

#include "models/Model.h"
#include "ort-utils/ORTModelData.h"

/**
 * @brief The filter_data_base struct
 * 
 * This struct is used to store the data needed for all filters.
*/
struct filter_data_base {
	obs_source_t *source;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;

	cv::Mat inputBGRA;

	bool isDisabled;

	std::mutex inputBGRALock;
	std::mutex outputLock;
};


/**
  * @brief The filter_data structs
  *
  * This struct is used to store the data needed for ONNX runtime filters.
  *
*/
struct filter_data : public ORTModelData, public filter_data_base {
	std::string useGPU;
	uint32_t numThreads;
	std::string modelSelection;
	std::unique_ptr<Model> model;

#if _WIN32
	const wchar_t *modelFilepath = nullptr;
#else
	const char *modelFilepath = nullptr;
#endif
};

#endif /* FILTERDATA_H */
