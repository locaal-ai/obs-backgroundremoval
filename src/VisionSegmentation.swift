import Foundation
import Vision
import AVFoundation
import CoreImage.CIFilterBuiltins

public class VisionSegmentation : NSObject {
    var originalImage: CIImage? = nil
    var mask: CVPixelBuffer? = nil
    let requestHandler = VNSequenceRequestHandler()
    lazy var segmentationRequest = {
        let segmentationRequest = VNGeneratePersonSegmentationRequest(completionHandler: self.handler(request:error:))
        segmentationRequest.qualityLevel = .accurate
        segmentationRequest.outputPixelFormat = kCVPixelFormatType_OneComponent32Float
        return segmentationRequest
    }()
    
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
        guard let segmantationRequest = request as? VNGeneratePersonSegmentationRequest else {
            print("VNRequest is not VNGeneratePersonSegmentationRequest")
            return
        }
        guard let originalImage = self.originalImage else {
            print("originalImage is nil")
            return
        }
        
        let rect = originalImage.extent
        let blackImage = CIImage(color: .black).cropped(to: rect)
        let whiteImage = CIImage(color: .white).cropped(to: rect)
        let whitePixelBuffer = imageToPixelBuffer(whiteImage.oriented(.left))!

        guard let maskPixelBuffer = segmantationRequest.results?.first?.pixelBuffer else {
            print("Result is not available")
            return
        }

        var maskImage = CIImage(cvPixelBuffer: maskPixelBuffer)
        
        let scaleX = originalImage.extent.width / maskImage.extent.width
        let scaleY = originalImage.extent.height / maskImage.extent.height
        maskImage = maskImage.transformed(by: .init(scaleX: scaleX, y: scaleY))

        let blendFilter = CIFilter.blendWithRedMask()
        blendFilter.inputImage = blackImage
        blendFilter.backgroundImage = whiteImage
        blendFilter.maskImage = maskImage
        
        guard let outputImage = blendFilter.outputImage else {
            print("Blend failed")
            return
        }
        guard let outputMask = imageToPixelBuffer(outputImage) else {
            print("CVPixelBuffer cannot be generated")
            return
        }
        DispatchQueue.main.async {
            self.mask = outputMask
        }
    }
    
    @objc
    public func process(_ buf: UnsafeMutableRawPointer, width: Int, height: Int, output: UnsafeMutableRawPointer) {
        var rawFramePixelBuffer: CVPixelBuffer?
        CVPixelBufferCreateWithBytes(nil, width, height, kCVPixelFormatType_32BGRA, buf, width * 4, nil, nil, nil, &rawFramePixelBuffer)
        let framePixelBuffer = rawFramePixelBuffer!
        let originalImage = CIImage(cvPixelBuffer: framePixelBuffer)
        self.originalImage = originalImage
        
        DispatchQueue.global(qos: .userInteractive).async {
            try? self.requestHandler.perform([self.segmentationRequest], on: framePixelBuffer)
        }
        
        guard let outputMask = self.mask else {
            print("a")
            return
        }
        
        let outputWidth = CVPixelBufferGetWidth(outputMask)
        let outputHeight = CVPixelBufferGetHeight(outputMask)
        if (outputWidth != width || outputHeight != height) {
            print("b")
            return
        }
        CVPixelBufferLockBaseAddress(outputMask, .readOnly)
        guard let source = CVPixelBufferGetBaseAddressOfPlane(outputMask, 0) else {
            print("c")
            return
        }
        output.copyMemory(from: source, byteCount: width * height)
        CVPixelBufferUnlockBaseAddress(outputMask, .readOnly)
    }
}
