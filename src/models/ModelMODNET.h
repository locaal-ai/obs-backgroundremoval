#ifndef MODELMODNET_H
#define MODELMODNET_H

#include "Model.h"

class ModelMODNET : public ModelBCHW {
  public:
  ModelMODNET(/* args */) {}
  ~ModelMODNET() {}

  virtual void prepareInputToNetwork(cv::Mat &resizedImage, cv::Mat &preprocessedImage)
  {
    cv::subtract(resizedImage, cv::Scalar::all(127.5), resizedImage);
    resizedImage = resizedImage / 127.5;
    hwc_to_chw(resizedImage, preprocessedImage);
  }
};

#endif // MODELMODNET_H
