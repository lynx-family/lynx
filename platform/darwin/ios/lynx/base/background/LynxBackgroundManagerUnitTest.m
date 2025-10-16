// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundManager.h>
#import <Lynx/LynxGradientUtils.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIView.h>
#import <XCTest/XCTest.h>
#import "LynxUI+Private.h"

@interface LynxBackgroundManagerUnitTest : XCTestCase {
  LynxUIView* _view;
}

@end

@implementation LynxBackgroundManagerUnitTest

- (void)setUp {
  _view = [[LynxUIView alloc] init];
  // overflow:visible
  [LynxPropsProcessor updateProp:@0 withKey:@"overflow" forUI:_view];
  // border-left-top-radius: 10px;
  [LynxPropsProcessor updateProp:@[ @10, @0, @10, @0 ]
                         withKey:@"border-left-top-radius"
                           forUI:_view];
  // boder-width: 1px;
  [LynxPropsProcessor updateProp:@1 withKey:@"border-left-width" forUI:_view];
  [LynxPropsProcessor updateProp:@1 withKey:@"border-right-width" forUI:_view];
  [LynxPropsProcessor updateProp:@1 withKey:@"border-top-width" forUI:_view];
  [LynxPropsProcessor updateProp:@1 withKey:@"border-bottom-width" forUI:_view];
  [_view propsDidUpdate];
  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, 100.0f, 100.0f)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];
}

- (void)tearDown {
  _view = nil;
}

- (void)testTopLayer {
  LynxUIView* parent = [[LynxUIView alloc] init];
  [parent insertChild:_view atIndex:0];
  CALayer* top = [_view topLayer];
  CALayer* realTop = [[[[parent view] layer] sublayers] lastObject];
  XCTAssertEqual(top, realTop);
}

- (void)testApplySimpleBorder {
  [LynxPropsProcessor updateProp:@[
    @75,
    @0,
    @75,
    @0,
    @75,
    @0,
    @75,
    @0,
    @75,
    @0,
    @75,
    @0,
    @75,
    @0,
    @75,
    @0,
  ]
                         withKey:@"border-radius"
                           forUI:_view];
  [_view propsDidUpdate];
  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, 100.0f, 200.0f)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];
  // border-radius: 75; border-width:1; border-color: black; overflow:visible;
  // Should have border layer and use layer props to draw borders.
  XCTAssertNotNil(_view.backgroundManager.borderLayer);
  XCTAssertEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
  XCTAssertEqual(_view.backgroundManager.borderLayer.borderWidth, 1);

  // If borderLayer is exist, border should apply on borderLayer not view.layer.
  XCTAssertEqual(_view.view.layer.borderWidth, 0);

  // CornerRadius should be adjust to half of the shorter edge's length.
  XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 50);
  XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 50);
}

- (void)testCalcBorder {
  [LynxPropsProcessor updateProp:@[
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
    @[ @0.1, @1, @10, @0 ],
    @2,
  ]
                         withKey:@"border-radius"
                           forUI:_view];
  [_view propsDidUpdate];
  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, 200.0f, 100.f)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];
  XCTAssertEqual(_view.backgroundManager.borderRadius.topLeftX.val, 30);
  XCTAssertEqual(_view.backgroundManager.borderRadius.topLeftX.unit, LynxBorderValueUnitDefault);
  XCTAssertEqual(_view.backgroundManager.borderRadius.topLeftY.val, 20);
  XCTAssertEqual(_view.backgroundManager.borderRadius.topLeftY.unit, LynxBorderValueUnitDefault);

  XCTAssertEqual(_view.backgroundManager.borderRadius.topRightX.val, 30);
  XCTAssertEqual(_view.backgroundManager.borderRadius.topRightX.unit, LynxBorderValueUnitDefault);
  XCTAssertEqual(_view.backgroundManager.borderRadius.topLeftY.val, 20);
  XCTAssertEqual(_view.backgroundManager.borderRadius.topRightY.unit, LynxBorderValueUnitDefault);

  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomLeftX.val, 30);
  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomLeftX.unit, LynxBorderValueUnitDefault);
  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomLeftY.val, 20);
  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomLeftY.unit, LynxBorderValueUnitDefault);

  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomRightX.val, 30);
  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomRightX.unit,
                 LynxBorderValueUnitDefault);
  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomRightY.val, 20);
  XCTAssertEqual(_view.backgroundManager.borderRadius.bottomRightY.unit,
                 LynxBorderValueUnitDefault);
}

// Tests background layer functionality with gradient image and solid color combination
// Verifies proper layer creation, property application, rendering, and cleanup behavior
- (void)testColorLayer {
  // Initialize length context for gradient calculations
  struct LynxLengthContext context = {};

  // Create gradient image from linear gradient string definition
  // Gradient: 180deg direction from #fe2c551f (red with alpha) to #fe2c5500 (transparent red)
  NSArray* image =
      [LynxGradientUtils getGradientArrayFromString:@"linear-gradient(180deg ,#fe2c551f ,#fe2c5500)"
                                  withLengthContext:context];

  // Apply gradient image as background-image property
  [LynxPropsProcessor updateProp:image withKey:@"background-image" forUI:_view];

  // Apply solid red background color (0xFFFF0000 = red in ARGB format)
  [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];

  [_view propsDidUpdate];

  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, 200.0f, 100.f)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];

  // Verify gradient image was properly applied to background layer
  XCTAssertNotNil(_view.backgroundManager.backgroundLayer.imageArray);

  // Verify background layer has sublayers (gradient + color layers)
  XCTAssertNotNil(_view.backgroundManager.backgroundLayer.sublayers);

  // Force layer to render
  [_view.backgroundManager.backgroundLayer display];

  // Verify first sublayer is a CAShapeLayer (used for solid color background)
  XCTAssertTrue([[_view.backgroundManager.backgroundLayer.sublayers objectAtIndex:0]
      isKindOfClass:[CAShapeLayer class]]);

  // Verify shape layer's fill color matches our specified red color
  XCTAssertTrue(CGColorEqualToColor(
      [(CAShapeLayer*)[_view.backgroundManager.backgroundLayer.sublayers objectAtIndex:0]
          fillColor],
      [UIColor redColor].CGColor));

  // Remove background color by setting to nil
  [LynxPropsProcessor updateProp:nil withKey:@"background-color" forUI:_view];
  [_view propsDidUpdate];
  [_view onNodeReadyForUIOwner];

  // Force layer to re-render after color removal
  [_view.backgroundManager.backgroundLayer display];

  // Verify color layer's fill color was properly removed
  CAShapeLayer* colorLayer = [_view.backgroundManager.backgroundLayer.sublayers objectAtIndex:0];
  XCTAssertTrue(colorLayer.fillColor == NULL);
}

// Tests that shadow layers are positioned correctly in the layer hierarchy
// Verifies shadow layers are above background color and gradient layers
- (void)testShadowLayerStackingOrder {
  // Initialize length context for gradient calculations
  struct LynxLengthContext context = {};

  // Create gradient image
  NSArray* image =
      [LynxGradientUtils getGradientArrayFromString:@"linear-gradient(180deg ,#fe2c551f ,#fe2c5500)"
                                  withLengthContext:context];

  UIView* parent = [UIView new];
  [parent addSubview:_view.view];

  [LynxPropsProcessor updateProp:@[ @[
                        @5.5,        // offset x
                        @5.5,        // offset y
                        @10.0,       // blur
                        @10.0,       // spread
                        @1,          // option
                        @0x80000000  // color
                      ] ]
                         withKey:@"box-shadow"
                           forUI:_view];

  [_view propsDidUpdate];

  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, 200.0f, 100.f)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];

  // Force layer to render
  [_view.backgroundManager.backgroundLayer display];
  NSArray* sublayers = _view.backgroundManager.backgroundLayer.sublayers;
  XCTAssertNotNil(sublayers);
  XCTAssertGreaterThan(sublayers.count, 0, @"Should have shadow layer");

  // Apply background color, gradient, and shadow
  [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];
  [LynxPropsProcessor updateProp:image withKey:@"background-image" forUI:_view];
  [_view propsDidUpdate];
  [_view onNodeReadyForUIOwner];
  // Force layer to render
  [_view.backgroundManager.backgroundLayer display];

  sublayers = _view.backgroundManager.backgroundLayer.sublayers;
  XCTAssertNotNil(sublayers);
  XCTAssertGreaterThan(sublayers.count, 2,
                       @"Should have at least color, gradient, and shadow layers");

  // Verify stacking order:
  // 1. Color layer should be at index 0 (bottom)
  CAShapeLayer* colorLayer = [sublayers objectAtIndex:0];
  XCTAssertTrue([colorLayer isKindOfClass:[CAShapeLayer class]],
                @"First layer should be color layer");
  XCTAssertTrue(CGColorEqualToColor(colorLayer.fillColor, [UIColor redColor].CGColor),
                @"Color layer should have red fill");

  // 2. Gradient layer should be above color layer
  CALayer* gradientLayer = [sublayers objectAtIndex:1];
  // Gradient layers are typically CATiledLayer or CAGradientLayer
  XCTAssertNotNil(gradientLayer, @"Should have gradient layer at index 1");
  XCTAssertTrue([gradientLayer isKindOfClass:[CAReplicatorLayer class]]);

  CALayer* shadowLayer = [sublayers objectAtIndex:2];
  XCTAssertTrue([shadowLayer shadowPath]);
  [_view.view removeFromSuperview];
}

@end
