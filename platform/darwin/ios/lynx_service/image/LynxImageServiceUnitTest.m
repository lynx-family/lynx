// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <ImageIO/ImageIO.h>
#import <SDWebImage/SDAnimatedImage.h>
#import <SDWebImage/SDAnimatedImageView.h>
#import <XCTest/XCTest.h>
#import "LynxImageService.h"

@interface LynxImageAnimationInfoUnitTest : XCTestCase
@end

@implementation LynxImageAnimationInfoUnitTest

- (SDAnimatedImage*)imageWithFrameDurations:(NSArray<NSNumber*>*)durations {
  UIGraphicsBeginImageContext(CGSizeMake(1, 1));
  NSMutableData* data = [NSMutableData new];
  CGImageDestinationRef destination = CGImageDestinationCreateWithData(
      (__bridge CFMutableDataRef)data, CFSTR("com.compuserve.gif"), durations.count, nil);
  for (NSUInteger index = 0; index < durations.count; ++index) {
    [[UIColor colorWithHue:(CGFloat)index / durations.count saturation:1 brightness:1
                     alpha:1] setFill];
    UIRectFill(CGRectMake(0, 0, 1, 1));
    UIImage* frame = UIGraphicsGetImageFromCurrentImageContext();
    NSDictionary* properties = @{
      (__bridge NSString*)kCGImagePropertyGIFDictionary : @{
        (__bridge NSString*)kCGImagePropertyGIFDelayTime : durations[index],
      },
    };
    CGImageDestinationAddImage(destination, frame.CGImage, (__bridge CFDictionaryRef)properties);
  }
  XCTAssertTrue(CGImageDestinationFinalize(destination));
  CFRelease(destination);
  UIGraphicsEndImageContext();
  SDAnimatedImage* image = [[SDAnimatedImage alloc] initWithData:data];
  XCTAssertNotNil(image);
  return image;
}

- (void)testSingleFrameImageReportsOneFrame {
  SDAnimatedImage* image = [self imageWithFrameDurations:@[ @0.1 ]];
  XCTAssertEqual(image.animatedImageFrameCount, 0u);
  SDAnimatedImageView* view = [SDAnimatedImageView new];
  view.image = image;
  NSDictionary* info = [[LynxImageService new] animationInfo:view];
  XCTAssertEqualObjects(info[@"frameCount"], @1);
  XCTAssertNil(info[@"loopDurationMs"]);
}

- (void)testAnimatedImageReportsOneLoopDuration {
  SDAnimatedImage* image = [self imageWithFrameDurations:@[ @0.1, @0.2, @0.3 ]];
  XCTAssertEqual(image.animatedImageFrameCount, 3u);
  XCTAssertEqual(image.duration, 0);
  SDAnimatedImageView* view = [SDAnimatedImageView new];
  view.image = image;
  NSDictionary* info = [[LynxImageService new] animationInfo:view];
  XCTAssertEqualObjects(info[@"frameCount"], @3);
  XCTAssertEqualWithAccuracy([info[@"loopDurationMs"] doubleValue], 600, 0.001);
}

@end
