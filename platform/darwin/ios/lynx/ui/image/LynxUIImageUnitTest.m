// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundManager.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIImage.h>
#import <XCTest/XCTest.h>

@interface LynxUIImage (UnitTest)
@property(nonatomic) UIImage* image;
- (void)onNodeReadyForUIOwner;
- (BOOL)isAnimated;
@end

@interface LynxUIImageUnitTest : XCTestCase

@property(nonatomic) LynxUIImage* UIImage;

@end

@implementation LynxUIImageUnitTest

- (void)setUp {
  _UIImage = [[LynxUIImage alloc] init];
  [LynxPropsProcessor updateProp:@1 withKey:@"ignore-cdn-downgrade-cache-policy" forUI:_UIImage];
  [LynxPropsProcessor updateProp:@1 withKey:@"ignore-memory-cache" forUI:_UIImage];
  [LynxPropsProcessor updateProp:@1 withKey:@"ignore-disk-cache" forUI:_UIImage];
  [LynxPropsProcessor updateProp:@1 withKey:@"not-cache-to-memory" forUI:_UIImage];
  [LynxPropsProcessor updateProp:@1 withKey:@"not-cache-to-disk" forUI:_UIImage];
  [_UIImage propsDidUpdate];
}

- (void)testAllSetOptions {
  LynxRequestOptions options = _UIImage.requestOptions;
  LynxRequestOptions testOptions =
      LynxImageDefaultOptions | LynxImageIgnoreMemoryCache | LynxImageIgnoreDiskCache |
      LynxImageNotCacheToDisk | LynxImageNotCacheToMemory | LynxImageIgnoreCDNDowngradeCachePolicy;
  XCTAssertEqual(options, testOptions);
}

- (void)testSetFalseSingleOption {
  [LynxPropsProcessor updateProp:@0 withKey:@"ignore-cdn-downgrade-cache-policy" forUI:_UIImage];
  [_UIImage propsDidUpdate];
  XCTAssertEqual(_UIImage.requestOptions, LynxImageDefaultOptions | LynxImageIgnoreMemoryCache |
                                              LynxImageIgnoreDiskCache | LynxImageNotCacheToDisk |
                                              LynxImageNotCacheToMemory);
  [LynxPropsProcessor updateProp:@0 withKey:@"ignore-memory-cache" forUI:_UIImage];
  [_UIImage propsDidUpdate];
  XCTAssertEqual(_UIImage.requestOptions, LynxImageDefaultOptions | LynxImageIgnoreDiskCache |
                                              LynxImageNotCacheToDisk | LynxImageNotCacheToMemory);
  [LynxPropsProcessor updateProp:@0 withKey:@"ignore-disk-cache" forUI:_UIImage];
  [_UIImage propsDidUpdate];
  XCTAssertEqual(_UIImage.requestOptions,
                 LynxImageDefaultOptions | LynxImageNotCacheToDisk | LynxImageNotCacheToMemory);
  [LynxPropsProcessor updateProp:@0 withKey:@"not-cache-to-memory" forUI:_UIImage];
  [_UIImage propsDidUpdate];
  XCTAssertEqual(_UIImage.requestOptions, LynxImageDefaultOptions | LynxImageNotCacheToDisk);
  [LynxPropsProcessor updateProp:@0 withKey:@"not-cache-to-disk" forUI:_UIImage];
  [_UIImage propsDidUpdate];
  XCTAssertEqual(_UIImage.requestOptions, LynxImageDefaultOptions);
}

- (void)testAnimatedImageBorderRadiusUpdateClearsMaskAtZero {
  UIGraphicsBeginImageContextWithOptions(CGSizeMake(1, 1), YES, 1);
  [[UIColor blackColor] setFill];
  UIRectFill(CGRectMake(0, 0, 1, 1));
  UIImage* frame = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  UIImage* animatedImage = [UIImage animatedImageWithImages:@[ frame, frame ] duration:1.0];
  _UIImage.image = animatedImage;
  XCTAssertNotNil(_UIImage.image.images);
  [_UIImage updateFrame:CGRectMake(0, 0, 100, 80)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];

  [LynxPropsProcessor
      updateProp:@[ @10, @0, @20, @0, @30, @0, @40, @0, @50, @0, @60, @0, @70, @0, @80, @0 ]
         withKey:@"border-radius"
           forUI:_UIImage];
  [_UIImage onNodeReadyForUIOwner];
  XCTAssertNotNil(_UIImage.view.layer.mask);

  _UIImage.image = animatedImage;
  [LynxPropsProcessor updateProp:@[ @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0 ]
                         withKey:@"border-radius"
                           forUI:_UIImage];
  XCTAssertTrue([_UIImage isAnimated]);
  XCTAssertNil(_UIImage.clipPath);
  XCTAssertFalse(LynxHasBorderRadii(_UIImage.backgroundManager.borderRadius));
  XCTAssertGreaterThan(_UIImage.nodeReadyBlockArray.count, 0U);
  [_UIImage onNodeReadyForUIOwner];
  XCTAssertNil(_UIImage.view.layer.mask);
}

@end
