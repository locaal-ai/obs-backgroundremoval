#ifndef MODELPPHUMANSEG_H
#define MODELPPHUMANSEG_H

#include "Model.h"

class ModelPPHumanSeg : public ModelBCHW {
  public:
  ModelPPHumanSeg(/* args */) {}
  ~ModelPPHumanSeg() {}

  virtual void prepareInputToNetwork(cv::Mat &resizedImage, cv::Mat &preprocessedImage)
  {
    // input_image = cv.resize(image, dsize=(input_size[1], input_size[0]))
    // mean = [0.5, 0.5, 0.5]
    // std = [0.5, 0.5, 0.5]
    // input_image = (input_image / 255 - mean) / std
    // input_image = input_image.transpose(2, 0, 1).astype('float32')
    // input_image = np.expand_dims(input_image, axis=0)

    resizedImage = (resizedImage / 256.0 - cv::Scalar(0.5, 0.5, 0.5)) / cv::Scalar(0.5, 0.5, 0.5);

    hwc_to_chw(resizedImage, preprocessedImage);
  }

  virtual cv::Mat getNetworkOutput(const std::vector<std::vector<int64_t>> &outputDims,
                                   std::vector<std::vector<float>> &outputTensorValues)
  {
    uint32_t outputWidth = (int)outputDims[0].at(2);
    uint32_t outputHeight = (int)outputDims[0].at(1);
    int32_t outputChannels = CV_32FC2;

    return cv::Mat(outputHeight, outputWidth, outputChannels, outputTensorValues[0].data());
  }

  virtual void postprocessOutput(cv::Mat &outputImage)
  {
    // take 1st channel
    std::vector<cv::Mat> outputImageSplit;
    cv::split(outputImage, outputImageSplit);

    // "Softmax"
    cv::Mat outputA, outputB;
    cv::exp(outputImageSplit[1], outputA);
    cv::exp(outputImageSplit[0], outputB);
    outputImage = outputA / (outputA + outputB);

    cv::normalize(outputImage, outputImage, 1.0, 0.0, cv::NORM_MINMAX);
  }

};

#endif // MODELPPHUMANSEG_H
