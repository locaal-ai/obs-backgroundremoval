import Foundation
import Vision
import AVFoundation
import CoreImage.CIFilterBuiltins


public class FacePoseSegmentation : NSObject {
    var originalImage: CIImage?
    var mask: CVPixelBuffer?
    var globalFramePixelBuffer: CVPixelBuffer?
    var requestHandler: VNSequenceRequestHandler?
    var segmentationRequest: VNGeneratePersonSegmentationRequest?
    
    func imageToPixelBuffer(_ image: CIImage) -> CVPixelBuffer? {
        var pixelBuffer: CVPixelBuffer?

        let width = Int(image.extent.width)
        let height = Int(image.extent.height)
        
        CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_OneComponent8, nil, &pixelBuffer)

        let context = CIContext()
        context.render(image, to: pixelBuffer!)
        return pixelBuffer
    }
    
    func handler(request: VNRequest, error: Error?) {
        guard let segmantationRequest = request as? VNGeneratePersonSegmentationRequest else { return }
        
        guard let originalImage = self.originalImage else { return }
        
        let rect = originalImage.extent
        let blackImage = CIImage(color: .black).cropped(to: rect)
        let whiteImage = CIImage(color: .white).cropped(to: rect)
        let whitePixelBuffer = imageToPixelBuffer(whiteImage.oriented(.left))!

        guard let maskPixelBuffer = segmantationRequest.results?.first?.pixelBuffer else { return }

        var maskImage = CIImage(cvPixelBuffer: maskPixelBuffer)
        
        let scaleX = originalImage.extent.width / maskImage.extent.width
        let scaleY = originalImage.extent.height / maskImage.extent.height
        maskImage = maskImage.transformed(by: .init(scaleX: scaleX, y: scaleY))

        let blendFilter = CIFilter.blendWithRedMask()
        blendFilter.inputImage = blackImage
        blendFilter.backgroundImage = whiteImage
        blendFilter.maskImage = maskImage
        
        guard let outputImage = blendFilter.outputImage else { return }
        guard let outputMask = imageToPixelBuffer(outputImage) else { return }
        self.mask = outputMask
    }
    
    func checkPixelBufferDimension(_ buf: CVPixelBuffer?, width: Int, height: Int) -> Bool {
        guard let b = buf else { return false }
        let bufWidth = CVPixelBufferGetWidth(b)
        let bufHeight = CVPixelBufferGetHeight(b)
        return bufWidth == width && bufHeight == height
    }
    
    func createFramePixelBuffer(expectedWidth: Int, expectedHeight: Int) -> CVPixelBuffer? {
        if (checkPixelBufferDimension(self.globalFramePixelBuffer, width: expectedWidth, height: expectedHeight)) {
            return self.globalFramePixelBuffer
        } else {
            CVPixelBufferCreate(nil, expectedWidth, expectedHeight, kCVPixelFormatType_32BGRA, nil, &self.globalFramePixelBuffer)
            self.requestHandler = VNSequenceRequestHandler()
            let segmentationRequest = VNGeneratePersonSegmentationRequest(completionHandler: self.handler(request:error:))
            segmentationRequest.qualityLevel = .fast
            segmentationRequest.outputPixelFormat = kCVPixelFormatType_OneComponent32Float
            self.segmentationRequest = segmentationRequest
            return self.globalFramePixelBuffer
        }
    }
    
    @objc
    public func process(_ buf: UnsafeMutableRawPointer, width: Int, height: Int, outputBuf: UnsafeMutableRawPointer) {
        guard let framePixelBuffer = createFramePixelBuffer(expectedWidth: width, expectedHeight: height) else { return }
        CVPixelBufferLockBaseAddress(framePixelBuffer, CVPixelBufferLockFlags(rawValue: 0))
        guard let dest = CVPixelBufferGetBaseAddressOfPlane(framePixelBuffer, 0) else { return }
        dest.copyMemory(from: buf, byteCount: width * height * 4)
        CVPixelBufferUnlockBaseAddress(framePixelBuffer, CVPixelBufferLockFlags(rawValue: 0))
        
        let originalImage = CIImage(cvPixelBuffer: framePixelBuffer)
        self.originalImage = originalImage

        guard let requestHandler = self.requestHandler else { return }
        guard let segmentationRequest = self.segmentationRequest else { return }

        DispatchQueue.global().async {
            try! requestHandler.perform([segmentationRequest], on: framePixelBuffer)
        }
        
        guard let outputMask = self.mask else { return }
        
        let resultWidth = CVPixelBufferGetWidth(outputMask)
        let resultHeight = CVPixelBufferGetHeight(outputMask)
        if (width == resultWidth && height == resultHeight) {
            CVPixelBufferLockBaseAddress(outputMask, .readOnly)
            guard let source = CVPixelBufferGetBaseAddressOfPlane(outputMask, 0) else { return }
            let bytesCount = resultWidth * resultHeight * 4
            outputBuf.copyMemory(from: source, byteCount: bytesCount)
            CVPixelBufferUnlockBaseAddress(outputMask, .readOnly)
        }
    }
}
