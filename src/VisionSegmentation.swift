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
        segmentationRequest.qualityLevel = .fast
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
        DispatchQueue.main.async {
            self.mask = outputMask
        }
    }
    
    @objc
    public func process(_ buf: UnsafeMutableRawPointer, width: Int, height: Int) -> CVPixelBuffer {
        var rawFramePixelBuffer: CVPixelBuffer?
        CVPixelBufferCreateWithBytes(nil, width, height, kCVPixelFormatType_32BGRA, buf, width * 4, nil, nil, nil, &rawFramePixelBuffer)
        let framePixelBuffer = rawFramePixelBuffer!
        let originalImage = CIImage(cvPixelBuffer: framePixelBuffer)
        self.originalImage = originalImage
        
        DispatchQueue.global().async {
            try! self.requestHandler.perform([self.segmentationRequest], on: framePixelBuffer)
        }
        
        guard let outputMask = self.mask else {
            let rect = originalImage.extent
            let whiteImage = CIImage(color: .white).cropped(to: rect)
            return imageToPixelBuffer(whiteImage.oriented(.left))!
        }
        
        return outputMask
    }
}
