#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "FacePoseSegmentation-Swift.h"

#include <opencv2/opencv.hpp>

void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask);

//==================================================
// [C++] Objective-C クラスのメソッドを呼ぶ
//==================================================
void processImageForBackgroundByVision(const cv::Mat& imageBGR, cv::Mat& backgroundMask)
{
    cv::Mat imageBGRA;
    cv::cvtColor(imageBGR, imageBGRA, cv::COLOR_BGR2BGRA);
    
    CVPixelBufferRef imagePixelBuffer;
    int imageWidth = imageBGR.cols;
    int imageHeight = imageBGR.rows;
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSNumber numberWithInt:imageWidth], kCVPixelBufferWidthKey,
                             [NSNumber numberWithInt:imageHeight], kCVPixelBufferHeightKey,
                             nil];
    CVReturn status = CVPixelBufferCreate(kCFAllocatorMalloc, imageWidth, imageHeight, kCVPixelFormatType_32BGRA, (CFDictionaryRef) CFBridgingRetain(options), &imagePixelBuffer);
    CVPixelBufferLockBaseAddress(imagePixelBuffer, 0);
    void *base = CVPixelBufferGetBaseAddress(imagePixelBuffer);
    memcpy(base, imageBGRA.data, imageBGRA.total()* imageBGRA.elemSize());
    CVPixelBufferUnlockBaseAddress(imagePixelBuffer, 0);
    
    FacePoseSegmentation *obj = [[FacePoseSegmentation alloc] init];
    CVPixelBufferRef resultPixelBuffer = [obj process:imagePixelBuffer];
    CVPixelBufferLockBaseAddress(resultPixelBuffer, 0);
    size_t width = CVPixelBufferGetWidth(resultPixelBuffer);
    size_t height = CVPixelBufferGetHeight(resultPixelBuffer);
    OSType format = CVPixelBufferGetPixelFormatType(resultPixelBuffer);
    void *baseaddress = CVPixelBufferGetBaseAddressOfPlane(resultPixelBuffer, 0);
    cv::Mat mat(height, width, CV_8U, baseaddress, 0);
    CVPixelBufferUnlockBaseAddress(resultPixelBuffer, 0);

    backgroundMask = mat;
}
