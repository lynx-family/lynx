// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxUIImage.h>
#import <XCTest/XCTest.h>
#import "LynxUIUnitTestUtils.h"

@interface LynxUIImage (RendererTesting)
- (void)requestImage;
- (void)requestImage:(LynxURL*)url;
- (void)onImageReady:(UIImage*)image withRequest:(LynxURL*)url;
- (void)sendImageResult:(LynxDetailEvent*)event;
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

- (LynxUIMockContext*)reloadContextForImage:(LynxUIImage*)image {
  LynxUIMockContext* context = [LynxUIUnitTestUtils initUIMockContextWithUI:image];
  [image setRawEvents:[NSSet setWithArray:@[ @"load(bindEvent)", @"error(bindEvent)" ]]
      andLepusRawEvents:[NSSet set]];
  [image onNodeReady];
  return context;
}

- (LynxDetailEvent*)loadResult {
  return [[LynxDetailEvent alloc] initWithName:@"load"
                                    targetSign:42
                                        detail:@{@"width" : @80, @"height" : @40, @"origin" : @1}];
}

- (void)testReloadReplaysResultWithoutPropertyChanges {
  LynxDetailEvent* error = [[LynxDetailEvent alloc] initWithName:@"error"
                                                      targetSign:42
                                                          detail:@{@"errMsg" : @"missing image"}];
  for (LynxDetailEvent* result in @[ [self loadResult], error ]) {
    LynxUIImageRequestProbe* image = [[LynxUIImageRequestProbe alloc] init];
    LynxUIMockContext* context = [self reloadContextForImage:image];
    LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:[LynxComponentScopeRegistry new]
                                                      screenMetrics:nil];
    image.sign = 42;
    NSMutableDictionary* uiHolder = [owner valueForKey:@"uiHolder"];
    uiHolder[@42] = image;
    image.requestCount = 0;
    [image sendImageResult:result];
    [image finishLayoutOperation];
    XCTAssertEqual(context.mockEventEmitter.cachedEvents.count, 1);
    [owner onNodeReload:42];
    [owner finishLayoutOperation:0 componentID:0];
    [owner finishLayoutOperation:0 componentID:0];
    XCTAssertEqual(context.mockEventEmitter.cachedEvents.count, 2);
    XCTAssertEqualObjects(context.mockEventEmitter.event.eventName, result.eventName);
    XCTAssertEqualObjects(context.mockEventEmitter.event.params, result.params);
    XCTAssertEqual(image.requestCount, 0);
  }
}

- (void)testReloadWithNewSourceDoesNotReplayOldResult {
  LynxUIImage* image = [[LynxUIImage alloc] init];
  LynxUIMockContext* context = [self reloadContextForImage:image];
  [image sendImageResult:[self loadResult]];
  [image onNodeReload];
  [LynxPropsProcessor updateProp:@"https://example.com/new.png" withKey:@"src" forUI:image];
  [image finishLayoutOperation];
  XCTAssertEqual(context.mockEventEmitter.cachedEvents.count, 1);
}

- (void)testRealCallbackConsumesReplayWithoutSuppressingLaterLoads {
  LynxUIImage* image = [[LynxUIImage alloc] init];
  LynxUIMockContext* context = [self reloadContextForImage:image];
  [image sendImageResult:[self loadResult]];
  [image onNodeReload];
  [image sendImageResult:[self loadResult]];
  [image finishLayoutOperation];
  XCTAssertEqual(context.mockEventEmitter.cachedEvents.count, 2);
  [image sendImageResult:[self loadResult]];
  XCTAssertEqual(context.mockEventEmitter.cachedEvents.count, 3);
}

@end
