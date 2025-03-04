// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxImageBlurUtils.h"

@implementation LynxImageBlurUtils

+ (UIImage *)blurImage:(UIImage *)inputImage withRadius:(CGFloat)radius {
  CGImageRef imageRef = inputImage.CGImage;
  UIImageOrientation imageOrientation = inputImage.imageOrientation;

  // Image must be nonzero size
  if (CGImageGetWidth(imageRef) * CGImageGetHeight(imageRef) == 0) {
    return inputImage;
  }

  // Convert to ARGB if it isn't
  if (CGImageGetBitsPerPixel(imageRef) != 32 || CGImageGetBitsPerComponent(imageRef) != 8 ||
      !((CGImageGetBitmapInfo(imageRef) & kCGBitmapAlphaInfoMask))) {
    UIGraphicsBeginImageContextWithOptions(inputImage.size, NO, inputImage.scale);
    [inputImage drawAtPoint:CGPointZero];
    imageRef = UIGraphicsGetImageFromCurrentImageContext().CGImage;
    UIGraphicsEndImageContext();
  }

  vImage_Buffer buffer1, buffer2;
  buffer1.width = buffer2.width = CGImageGetWidth(imageRef);
  buffer1.height = buffer2.height = CGImageGetHeight(imageRef);
  buffer1.rowBytes = buffer2.rowBytes = CGImageGetBytesPerRow(imageRef);
  size_t bytes = buffer1.rowBytes * buffer1.height;
  buffer1.data = malloc(bytes);
  buffer2.data = malloc(bytes);

  if (!buffer1.data || !buffer2.data) {
    if (buffer1.data) {
      free(buffer1.data);
    }
    if (buffer2.data) {
      free(buffer2.data);
    }
    return inputImage;
  }

  // A description of how to compute the box kernel width from the Gaussian
  // radius (aka standard deviation) appears in the SVG spec:
  // https://drafts.fxtf.org/filter-effects/#feGaussianBlurElement
  CGFloat inputRadius = radius * [[UIScreen mainScreen] scale];
  uint32_t boxSize = floor(inputRadius * 3 * sqrt(2 * M_PI) / 4 + 0.5);
  // Force boxSize to be odd
  boxSize |= 1;

  // Create temp buffer to avoid lost time due to VM faults to initialize newly allocated buffers.
  void *tempBuffer = malloc(
      (size_t)vImageBoxConvolve_ARGB8888(&buffer1, &buffer2, NULL, 0, 0, boxSize, boxSize, NULL,
                                         kvImageEdgeExtend + kvImageGetTempBufferSize));

  if (!tempBuffer) {
    free(buffer1.data);
    free(buffer2.data);
    return inputImage;
  }

  // Copy image data
  CFDataRef dataSource = CGDataProviderCopyData(CGImageGetDataProvider(imageRef));
  memcpy(buffer1.data, CFDataGetBytePtr(dataSource), bytes);
  CFRelease(dataSource);

  // Perform blur
  vImageBoxConvolve_ARGB8888(&buffer1, &buffer2, tempBuffer, 0, 0, boxSize, boxSize, NULL,
                             kvImageEdgeExtend);
  vImageBoxConvolve_ARGB8888(&buffer2, &buffer1, tempBuffer, 0, 0, boxSize, boxSize, NULL,
                             kvImageEdgeExtend);
  vImageBoxConvolve_ARGB8888(&buffer1, &buffer2, tempBuffer, 0, 0, boxSize, boxSize, NULL,
                             kvImageEdgeExtend);

  // Free buffers
  free(buffer2.data);
  free(tempBuffer);

  // Create image context from buffer
  CGContextRef ctx =
      CGBitmapContextCreate(buffer1.data, buffer1.width, buffer1.height, 8, buffer1.rowBytes,
                            CGImageGetColorSpace(imageRef), CGImageGetBitmapInfo(imageRef));

  // Create image from context
  imageRef = CGBitmapContextCreateImage(ctx);
  UIImage *outputImage = [UIImage imageWithCGImage:imageRef
                                             scale:inputImage.scale
                                       orientation:imageOrientation];
  CGImageRelease(imageRef);
  CGContextRelease(ctx);
  free(buffer1.data);
  return outputImage;
}

@end
