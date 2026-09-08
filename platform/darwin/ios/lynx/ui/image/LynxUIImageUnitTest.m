// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxUIImage.h>
#import <XCTest/XCTest.h>

@interface LynxUIImage (RendererTesting)
- (void)requestImage;
- (void)requestImage:(LynxURL*)url;
- (void)onImageReady:(UIImage*)image withRequest:(LynxURL*)url;
@end

@interface LynxUIImageRequestProbe : LynxUIImage
@property(nonatomic) NSInteger requestCount;
@end

@implementation LynxUIImageRequestProbe
- (void)requestImage:(LynxURL*)url {
  self.requestCount++;
}
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

- (void)testRendererHostDoesNotRequestImages {
  LynxUIImageRequestProbe* image = [[LynxUIImageRequestProbe alloc] init];
  id<LynxRendererHost> host = (id<LynxRendererHost>)image.view;
  host.renderer = [host createRendererWithSign:1 andContext:nil];
  [image requestImage];
  XCTAssertEqual(image.requestCount, 0);
}

- (void)testOrdinaryImageRequestsSourceAndPlaceholder {
  LynxUIImageRequestProbe* image = [[LynxUIImageRequestProbe alloc] init];
  [image requestImage];
  XCTAssertEqual(image.requestCount, 2);
}

- (void)testRendererHostIgnoresCompletedImage {
  id<LynxRendererHost> host = (id<LynxRendererHost>)_UIImage.view;
  host.renderer = [host createRendererWithSign:1 andContext:nil];
  [_UIImage onImageReady:[[UIImage alloc] init] withRequest:nil];
  XCTAssertNil(_UIImage.view.image);
  XCTAssertNil(_UIImage.view.animationImages);
}

- (void)testOrdinaryImageDisplaysCompletedImage {
  UIImage* result = [[UIImage alloc] init];
  [_UIImage onImageReady:result withRequest:nil];
  XCTAssertEqual(_UIImage.view.image, result);
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

@end
