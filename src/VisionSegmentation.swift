import Foundation
import Vision
import AVFoundation
import CoreImage.CIFilterBuiltins

public class VisionSegmentation : NSObject {
    var originalFrameBuffer: CVPixelBuffer?
    var outputMask: CVPixelBuffer?
    var isProcessing: Bool = false
    var requestHandler: VNSequenceRequestHandler?
    var segmentationRequest: VNGeneratePersonSegmentationRequest?
    
    func createPixelBuffer(width: Int, height: Int, pixelFormatType: OSType) -> CVPixelBuffer? {
        var pixelBuffer: CVPixelBuffer?
        CVPixelBufferCreate(nil, width, height, pixelFormatType, nil, &pixelBuffer)
        return pixelBuffer
    }
    
    func convertImageToPixelBuffer(image: CIImage, pixelFormatType: OSType) -> CVPixelBuffer? {
        let width = Int(image.extent.width)
        let height = Int(image.extent.height)
        guard let pixelBuffer = createPixelBuffer(width: width, height: height, pixelFormatType: pixelFormatType) else { return nil }
        let context = CIContext()
        context.render(image, to: pixelBuffer)
        return pixelBuffer
    }
    
    func handler(request: VNRequest, error: Error?) {
        guard let segmantationRequest = request as? VNGeneratePersonSegmentationRequest else {
            print("VNRequest is not VNGeneratePersonSegmentationRequest")
            return
        }
        
        guard let pixelBuffer = segmantationRequest.results?.first?.pixelBuffer else {
            print("Segumentation result is not available")
            return
        }
        
        guard let originalFrameBuffer = self.originalFrameBuffer else {
            print("OriginalFrameBuffer is not available")
            return
        }
        
        let maskImage = CIImage(cvImageBuffer: pixelBuffer)
        let originalImage = CIImage(cvPixelBuffer: originalFrameBuffer)
        let rect = originalImage.extent
        
        guard let outputImage = blend(rect: rect, maskImage: maskImage) else {
            print("Blend failed")
            return
        }
        guard let outputMask = convertImageToPixelBuffer(image: outputImage, pixelFormatType: kCVPixelFormatType_OneComponent8) else {
            print("CVPixelBuffer cannot be generated")
            return
        }
        DispatchQueue.main.async {
            self.outputMask = outputMask
            self.isProcessing = false
        }
    }
    
    func blend(rect: CGRect, maskImage: CIImage) -> CIImage? {
        let blackImage = CIImage(color: .black).cropped(to: rect)
        let whiteImage = CIImage(color: .white).cropped(to: rect)
        
        let scaleX = rect.width / maskImage.extent.width
        let scaleY = rect.height / maskImage.extent.height
        let transformedMaskImage = maskImage.transformed(by: .init(scaleX: scaleX, y: scaleY))

        let blendFilter = CIFilter.blendWithRedMask()
        blendFilter.inputImage = blackImage
        blendFilter.backgroundImage = whiteImage
        blendFilter.maskImage = transformedMaskImage
        
        return blendFilter.outputImage
    }
    
    func createPixelBufferFromBGRABytes(buf: UnsafeMutableRawPointer, width: Int, height: Int) -> CVPixelBuffer? {
        var framePixelBuffer: CVPixelBuffer?
        CVPixelBufferCreateWithBytes(nil, width, height, kCVPixelFormatType_32BGRA, buf, width * 4, nil, nil, nil, &framePixelBuffer)
        return framePixelBuffer
    }
    
    @objc
    public func process(_ buf: UnsafeMutableRawPointer, width: Int, height: Int, output: UnsafeMutableRawPointer) {
        guard let framePixelBuffer = createPixelBufferFromBGRABytes(buf: buf, width: width, height: width) else {
            print("PixelBuffer cannot be allocated")
            return
        }
        self.originalFrameBuffer = framePixelBuffer
        
        DispatchQueue.main.async {
            let requestHandler = VNSequenceRequestHandler()
            self.requestHandler = requestHandler
            let segmentationRequest = VNGeneratePersonSegmentationRequest(completionHandler: self.handler(request:error:))
            segmentationRequest.qualityLevel = .fast
            segmentationRequest.outputPixelFormat = kCVPixelFormatType_OneComponent32Float
            self.segmentationRequest = segmentationRequest
            try? requestHandler.perform([segmentationRequest], on: framePixelBuffer)
        }
        
        guard let outputMask = self.outputMask else {
            print("OutputMask is not avaiable")
            return
        }
        
        CVPixelBufferLockBaseAddress(outputMask, .readOnly)
        guard let source = CVPixelBufferGetBaseAddressOfPlane(outputMask, 0) else {
            print("PixelBuffer cannot be read")
            return
        }
        output.copyMemory(from: source, byteCount: width * height)
        CVPixelBufferUnlockBaseAddress(outputMask, .readOnly)
    }
}
