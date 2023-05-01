#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs-module.h>

#include "models/Model.h"
#include "ort-utils/ORTModelData.h"

struct filter_data : public ORTModelData {
  std::string useGPU;
  std::string modelSelection;
  std::unique_ptr<Model> model;

  obs_source_t *source;
  gs_texrender_t *texrender;
  gs_stagesurf_t *stagesurface;

  cv::Mat inputBGRA;

  bool isDisabled;

  std::mutex inputBGRALock;
  std::mutex outputLock;

#if _WIN32
  const wchar_t *modelFilepath = nullptr;
#else
  const char *modelFilepath = nullptr;
#endif
};

#endif /* FILTERDATA_H */
