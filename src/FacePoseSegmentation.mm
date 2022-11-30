#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "FacePoseSegmentation-Swift.h"

#include <opencv2/opencv.hpp>

static FacePoseSegmentation *obj = [[FacePoseSegmentation alloc] init];

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask);

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask)
{
    cv::Mat imageBGRA;
    cv::cvtColor(imageBGR, imageBGRA, cv::COLOR_BGR2BGRA);
    
    CVPixelBufferRef imagePixelBuffer;
    int imageWidth = imageBGR.cols;
    int imageHeight = imageBGR.rows;
    CVReturn status = CVPixelBufferCreate(kCFAllocatorMalloc, imageWidth, imageHeight, kCVPixelFormatType_32BGRA, nil, &imagePixelBuffer);
    CVPixelBufferLockBaseAddress(imagePixelBuffer, 0);
    void *base = CVPixelBufferGetBaseAddress(imagePixelBuffer);
    memcpy(base, imageBGRA.data, imageBGRA.total() * imageBGRA.elemSize());
    CVPixelBufferUnlockBaseAddress(imagePixelBuffer, 0);
    
    CVPixelBufferRef resultPixelBuffer = [obj process:imagePixelBuffer];
    CVPixelBufferLockBaseAddress(resultPixelBuffer, 0);
    size_t width = CVPixelBufferGetWidth(resultPixelBuffer);
    size_t height = CVPixelBufferGetHeight(resultPixelBuffer);
    void *baseaddress = CVPixelBufferGetBaseAddressOfPlane(resultPixelBuffer, 0);
    cv::Mat mat((int)height, (int)width, CV_8U, baseaddress, 0);
    CVPixelBufferUnlockBaseAddress(resultPixelBuffer, 0);

    backgroundMask = mat;
}
