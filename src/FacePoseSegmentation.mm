#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "FacePoseSegmentation-Swift.h"

#include <opencv2/opencv.hpp>

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask);

static FacePoseSegmentation *obj = [[FacePoseSegmentation alloc] init];

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask)
{
    cv::Mat imageBGRA;
    cv::cvtColor(imageBGR, imageBGRA, cv::COLOR_BGR2BGRA);
    
    CVPixelBufferRef imagePixelBuffer;
    int imageWidth = imageBGR.cols;
    int imageHeight = imageBGR.rows;
    
    [obj process:imageBGRA.data width:imageWidth height:imageHeight outputBuf:backgroundMask.data];
}
