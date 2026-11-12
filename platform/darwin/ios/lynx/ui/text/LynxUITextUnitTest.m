// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxTextView.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIText.h>
#import <XCTest/XCTest.h>

@interface LynxUIText (DrawingTesting)
- (void)requestDisplay;
- (void)requestDisplayAsynchronously;
@end

@interface LynxUITextDisplayProbe : LynxUIText
@property(nonatomic, copy) lynx_async_display_completion_block_t pendingDisplay;
@end

@implementation LynxUITextDisplayProbe
- (BOOL)enableLayerRender {
  return YES;
}
- (void)displayAsyncWithCompletionBlock:(lynx_async_display_completion_block_t)block {
  self.pendingDisplay = block;
}
@end

@interface LynxUITextRendererProbe : LynxTextRenderer
@property(nonatomic) NSInteger drawCount;
@end

@implementation LynxUITextRendererProbe
- (void)drawRect:(CGRect)bounds padding:(UIEdgeInsets)padding border:(UIEdgeInsets)border {
  self.drawCount++;
}
@end

@interface LynxUITextUnitTest : XCTestCase
@end

@implementation LynxUITextUnitTest

- (void)testLegacyDrawingFollowsRendererOwnership {
  for (NSNumber *fragmentRendering in @[ @NO, @YES ]) {
    LynxUITextDisplayProbe *text = [LynxUITextDisplayProbe new];
    LynxUITextRendererProbe *renderer = [LynxUITextRendererProbe new];
    id<LynxRendererHost> host = (id<LynxRendererHost>)text.view;
    if (fragmentRendering.boolValue) {
      host.renderer = [host createRendererWithSign:1 andContext:nil];
    }
    [text onReceiveUIOperation:renderer];
    text.overflow = OVERFLOW_XY_VAL;
    [text requestDisplay];
    XCTAssertEqual([text valueForKey:@"overflow_layer"] == nil, fragmentRendering.boolValue);

    text.overflow = OVERFLOW_HIDDEN_VAL;
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(10, 10), NO, 1);
    [text.view drawRect:CGRectMake(0, 0, 10, 10)];
    UIGraphicsEndImageContext();
    XCTAssertEqual(renderer.drawCount, fragmentRendering.boolValue ? 0 : 1);
    XCTAssertEqual(text.view.textRenderer, renderer);
  }
}

- (void)testPendingLegacyImageFollowsRendererOwnership {
  UIGraphicsBeginImageContextWithOptions(CGSizeMake(1, 1), NO, 1);
  UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  for (NSNumber *fragmentRendering in @[ @NO, @YES ]) {
    LynxUITextDisplayProbe *text = [LynxUITextDisplayProbe new];
    [text requestDisplayAsynchronously];
    XCTAssertNotNil(text.pendingDisplay);
    if (fragmentRendering.boolValue) {
      id<LynxRendererHost> host = (id<LynxRendererHost>)text.view;
      host.renderer = [host createRendererWithSign:1 andContext:nil];
    }
    text.pendingDisplay(image);
    CALayer *layer = [text valueForKey:@"overflow_layer"];
    XCTAssertEqual(layer.contents == nil, fragmentRendering.boolValue);
  }
}

@end
