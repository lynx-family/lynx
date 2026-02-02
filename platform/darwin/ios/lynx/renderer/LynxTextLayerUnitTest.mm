// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTextLayer.h>
#import <Lynx/LynxTextRenderManager.h>  // Assuming LynxTextRenderer is defined here or imported
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

// Forward declare LynxTextRenderer if header not available, or import it.
// Assuming LynxTextRenderer is a class.
@class LynxTextRenderer;

@interface LynxTextLayerUnitTest : XCTestCase
@end

@implementation LynxTextLayerUnitTest

- (void)testInitWithLynxTextRenderer {
  id mockTextRenderer = OCMClassMock([LynxTextRenderer class]);
  LynxTextLayer *layer = [[LynxTextLayer alloc] initWithLynxTextRenderer:mockTextRenderer];

  XCTAssertNotNil(layer);
  XCTAssertTrue([layer.delegate isEqual:layer]);
}

- (void)testDrawInContext {
  id mockTextRenderer = OCMClassMock([LynxTextRenderer class]);
  LynxTextLayer *layer = [[LynxTextLayer alloc] initWithLynxTextRenderer:mockTextRenderer];

  // Set a frame to ensure non-zero rect
  layer.frame = CGRectMake(0, 0, 100, 50);

  // Expect drawRect:padding:border: to be called
  [[mockTextRenderer expect] drawRect:CGRectMake(0, 0, 100, 50)
                              padding:UIEdgeInsetsZero
                               border:UIEdgeInsetsZero];

  // Create a graphics context to draw into
  UIGraphicsBeginImageContext(CGSizeMake(100, 50));
  CGContextRef ctx = UIGraphicsGetCurrentContext();

  [layer drawInContext:ctx];

  UIGraphicsEndImageContext();

  [mockTextRenderer verify];
}

@end
