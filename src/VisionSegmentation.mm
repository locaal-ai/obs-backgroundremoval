#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "VisionSegmentation-Swift.h"

#include <opencv2/opencv.hpp>

static VisionSegmentation *obj = [[VisionSegmentation alloc] init];

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask);

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask)
{
    cv::Mat imageBGRA;
    cv::cvtColor(imageBGR, imageBGRA, cv::COLOR_BGR2BGRA);

    int imageWidth = imageBGR.cols;
    int imageHeight = imageBGR.rows;
    
    cv::Mat mat((int)imageHeight, (int)imageWidth, CV_8U);
    [obj process:imageBGRA.data width:imageWidth height:imageHeight output:mat.data];
    backgroundMask = mat;
}
