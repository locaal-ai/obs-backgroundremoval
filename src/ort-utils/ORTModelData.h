#ifndef ORTMODELDATA_H
#define ORTMODELDATA_H

#include <onnxruntime_cxx_api.h>

struct ORTModelData {
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
};

#endif /* ORTMODELDATA_H */
