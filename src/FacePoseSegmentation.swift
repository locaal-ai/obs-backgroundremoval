import Foundation
import Vision
import AVFoundation
import CoreImage.CIFilterBuiltins

public class FacePoseSegmentation : NSObject {
    public static let requestHandler = VNSequenceRequestHandler()
    public static let segmentationRequest: VNGeneratePersonSegmentationRequest = {
        let req = VNGeneratePersonSegmentationRequest()
        req.qualityLevel = .fast
        req.outputPixelFormat = kCVPixelFormatType_OneComponent32Float
        return req
    }()
        
    func imageToPixelBuffer(_ image: CIImage) -> CVPixelBuffer? {
        var pixelBuffer: CVPixelBuffer?

        let width = Int(image.extent.width)
        let height = Int(image.extent.height)
        
        CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_OneComponent8, nil, &pixelBuffer)

        // put bytes into pixelBuffer
        let context = CIContext()
        context.render(image, to: pixelBuffer!)
        return pixelBuffer
    }
    
    @objc
    public func process(_ framePixelBuffer: CVPixelBuffer) -> CVPixelBuffer {
        let originalImage = CIImage(cvPixelBuffer: framePixelBuffer)

        let rect = originalImage.extent

        let blackImage = CIImage(color: .black).cropped(to: rect)
        let whiteImage = CIImage(color: .white).cropped(to: rect)
        let whitePixelBuffer = imageToPixelBuffer(whiteImage.oriented(.left))!
        
        try! FacePoseSegmentation.requestHandler.perform(
            [FacePoseSegmentation.segmentationRequest],
            on: framePixelBuffer)

        let maskPixelBuffer = FacePoseSegmentation.segmentationRequest.results?.first?.pixelBuffer

        var maskImage = CIImage(cvPixelBuffer: maskPixelBuffer!)

        let scaleX = originalImage.extent.width / maskImage.extent.width
        let scaleY = originalImage.extent.height / maskImage.extent.height
        maskImage = maskImage.transformed(by: .init(scaleX: scaleX, y: scaleY))

        let blendFilter = CIFilter.blendWithRedMask()
        blendFilter.inputImage = blackImage
        blendFilter.backgroundImage = whiteImage
        blendFilter.maskImage = maskImage
        
        return imageToPixelBuffer(blendFilter.outputImage!)!
    }
}
