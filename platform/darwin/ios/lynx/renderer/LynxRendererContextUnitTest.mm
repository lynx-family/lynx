// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxTextRenderManager.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#include "core/renderer/ui_wrapper/painting/paint_image.h"

@interface LynxRendererContextUnitTest : XCTestCase
@end

@implementation LynxRendererContextUnitTest

- (void)testProperties {
  LynxRendererContext *context = [[LynxRendererContext alloc] init];

  // Test bodyView property
  id mockBodyView = OCMProtocolMock(@protocol(LUIBodyView));
  context.bodyView = mockBodyView;
  XCTAssertEqual(context.bodyView, mockBodyView);

  // Test textRenderManager property
  id mockTextRenderManager = OCMClassMock([LynxTextRenderManager class]);
  context.textRenderManager = mockTextRenderManager;
  XCTAssertEqual(context.textRenderManager, mockTextRenderManager);
}

- (void)testImageStreamsFramesAndBackgroundPreloadsFrames {
  id loader = OCMClassMock([LynxImageLoader class]);
  OCMStub(ClassMethod([loader sharedInstance])).andReturn(loader);
  NSMutableArray<NSNumber *> *preloadFlags = [NSMutableArray new];
  OCMStub([loader loadImageWithOptions:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    __unsafe_unretained LynxImageLoadOptions *options = nil;
    [invocation getArgument:&options atIndex:2];
    [preloadFlags addObject:options.contextInfo[LynxImagePreloadAllFrames]];
  });
  LynxRendererContext *context = [LynxRendererContext new];
  LynxURL *source = [LynxURL new];
  source.url = [NSURL URLWithString:@"https://example.com/image.gif"];
  source.imageSize = CGSizeMake(100, 80);
  source.type = LynxImageRequestSrc;
  LynxURL *placeholder = [LynxURL new];
  placeholder.url = [NSURL URLWithString:@"https://example.com/placeholder.gif"];
  placeholder.imageSize = source.imageSize;
  placeholder.type = LynxImageRequestPlaceholder;
  for (NSNumber *background in @[ @NO, @YES ]) {
    lynx::tasm::ImagePaintInfo info;
    info.is_background = background.boolValue;
    [context createImageManager:7
                  withSourceURL:source
              andPlaceholderURL:placeholder
                      paintInfo:info
                      eventMask:0
                       imageKey:background.intValue];
  }
  XCTAssertEqualObjects(preloadFlags, (@[ @NO, @NO, @YES, @YES ]));
  [loader stopMocking];
}

@end
