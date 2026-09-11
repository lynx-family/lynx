// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundManager.h>
#import <Lynx/LynxGradientUtils.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIView.h>
#import <XCTest/XCTest.h>

@interface LynxBackgroundManagerUnitTest : XCTestCase {
  LynxUIView* _view;
}

@end

@implementation LynxBackgroundManagerUnitTest

- (void)setUp {
  _view = [[LynxUIView alloc] init];
  // overflow:visible
  [LynxPropsProcessor updateProp:@0 withKey:@"overflow" forUI:_view];
  // border-top-left-radius: 10px;
  [LynxPropsProcessor updateProp:@[ @10, @0, @10, @0 ]
                         withKey:@"border-top-left-radius"
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

- (void)testBackgroundColorClipUsesBottomImageLayerClip {
  [LynxPropsProcessor updateProp:@[
    @((NSUInteger)LynxBackgroundImageNone),
    @((NSUInteger)LynxBackgroundImageNone),
  ]
                         withKey:@"background-image"
                           forUI:_view];
  [LynxPropsProcessor updateProp:@[
    @((NSInteger)LynxBackgroundClipBorderBox),
    @((NSInteger)LynxBackgroundClipContentBox),
    @((NSInteger)LynxBackgroundClipBorderBox),
  ]
                         withKey:@"background-clip"
                           forUI:_view];
  [LynxPropsProcessor updateProp:@0xFF00FF00 withKey:@"background-color" forUI:_view];

  [_view propsDidUpdate];
  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, 200.0f, 100.f)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];

  XCTAssertNotNil(_view.backgroundManager.backgroundLayer);
  XCTAssertEqual(_view.backgroundManager.backgroundLayer.backgroundColorClip,
                 LynxBackgroundClipContentBox);
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

// The order deliberately includes subsets with a square top-left corner.
- (void)setCornerValues:(NSArray<NSArray*>*)values {
  NSArray* keys = @[
    @"border-top-left-radius", @"border-top-right-radius", @"border-bottom-left-radius",
    @"border-bottom-right-radius"
  ];
  for (NSUInteger i = 0; i < keys.count; ++i) {
    [LynxPropsProcessor updateProp:values[i] withKey:keys[i] forUI:_view];
  }
  [_view propsDidUpdate];
  [_view onNodeReadyForUIOwner];
}

- (void)setCornerSubset:(NSUInteger)subset radius:(NSNumber*)radius {
  NSMutableArray* values = [NSMutableArray array];
  for (NSUInteger i = 0; i < 4; ++i) {
    NSNumber* value = (subset & (1 << i)) ? radius : @0;
    [values addObject:@[ value, @0, value, @0 ]];
  }
  [self setCornerValues:values];
}

- (void)layoutWithSize:(CGSize)size {
  [_view updateFrameWithoutLayoutAnimation:CGRectMake(0, 0, size.width, size.height)
                               withPadding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero
                                    margin:UIEdgeInsetsZero];
  [_view frameDidChange];
  [_view onNodeReadyForUIOwner];
}

- (void)testMaskedCornerSubsetsAndPaintingMigration {
  if (@available(iOS 11.0, *)) {
    CACornerMask corners[] = {kCALayerMinXMinYCorner, kCALayerMaxXMinYCorner,
                              kCALayerMinXMaxYCorner, kCALayerMaxXMaxYCorner};
    [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];
    // Visible overflow paints on siblings; hidden overflow paints on the view.
    for (NSNumber* overflow in
         @[ @(LynxOverflowVisible), @(LynxOverflowHidden), @(LynxOverflowVisible) ]) {
      [LynxPropsProcessor updateProp:overflow withKey:@"overflow" forUI:_view];
      for (NSUInteger subset = 0; subset < 16; ++subset) {
        [self setCornerSubset:subset radius:@10];
        CACornerMask expected = 0;
        for (NSUInteger i = 0; i < 4; ++i) {
          if (subset & (1 << i)) expected |= corners[i];
        }
        if (subset == 0) expected = corners[0] | corners[1] | corners[2] | corners[3];
        XCTAssertEqual(_view.view.layer.cornerRadius, subset ? 10 : 0);
        XCTAssertEqual(_view.view.layer.maskedCorners, expected);
        XCTAssertEqual([_view.backgroundManager hasDifferentBorderRadius],
                       subset != 0 && subset != 15);
        if (overflow.intValue == LynxOverflowHidden) {
          XCTAssertNil(_view.backgroundManager.borderLayer);
          XCTAssertNil(_view.backgroundManager.backgroundLayer);
          XCTAssertNil(_view.view.layer.mask);
          XCTAssertTrue(_view.view.clipsToBounds);
          XCTAssertEqual(_view.view.layer.borderWidth, 1);
          XCTAssertTrue(
              CGColorEqualToColor(_view.view.layer.backgroundColor, UIColor.redColor.CGColor));
        } else {
          CALayer* border = _view.backgroundManager.borderLayer;
          CALayer* background = _view.backgroundManager.backgroundLayer;
          XCTAssertEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
          XCTAssertEqual(_view.backgroundManager.backgroundLayer.type, LynxBgTypeSimple);
          XCTAssertEqual(border.cornerRadius, subset ? 10 : 0);
          XCTAssertEqual(background.cornerRadius, subset ? 10 : 0);
          XCTAssertEqual(border.maskedCorners, expected);
          XCTAssertEqual(background.maskedCorners, expected);
          XCTAssertEqual(border.borderWidth, 1);
          XCTAssertEqual(_view.view.layer.borderWidth, 0);
          XCTAssertFalse(border.masksToBounds);
          XCTAssertFalse(background.masksToBounds);
        }
      }
    }
  }
}

- (void)testComplexRadiusFallbackAndReset {
  if (@available(iOS 11.0, *)) {
    [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];
    NSArray* zero = @[ @0, @0, @0, @0 ];
    NSArray* round = @[ @10, @0, @10, @0 ];
    for (NSArray* values in @[
           @[ @[ @10, @0, @20, @0 ], zero, zero, zero ],   // ellipse
           @[ round, @[ @20, @0, @20, @0 ], zero, zero ],  // differing circles
           @[ @[ @75, @0, @75, @0 ], zero, zero, zero ]    // oversized partial
         ]) {
      [self setCornerSubset:2 radius:@10];
      [self setCornerValues:values];
      XCTAssertEqual(_view.view.layer.cornerRadius, 0);
      XCTAssertEqual(_view.backgroundManager.backgroundLayer.cornerRadius, 0);
      XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 0);
      CACornerMask all = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner | kCALayerMinXMaxYCorner |
                         kCALayerMaxXMaxYCorner;
      XCTAssertEqual(_view.view.layer.maskedCorners, all);
      XCTAssertEqual(_view.backgroundManager.backgroundLayer.maskedCorners, all);
      XCTAssertEqual(_view.backgroundManager.borderLayer.maskedCorners, all);
      XCTAssertNotEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
      XCTAssertEqual(_view.backgroundManager.backgroundLayer.type, LynxBgTypeComplex);
    }
    // A corner with a zero axis is square, not an ellipse.
    [self setCornerValues:@[ @[ @0, @0, @20, @0 ], round, zero, zero ]];
    XCTAssertEqual(_view.view.layer.cornerRadius, 10);
    XCTAssertEqual(_view.view.layer.maskedCorners, kCALayerMaxXMinYCorner);
  }
}

- (void)testThickPartialBorderKeepsNormalizedPainting {
  if (@available(iOS 11.0, *)) {
    [self setCornerSubset:2 radius:@10];
    for (NSString* key in @[
           @"border-left-width", @"border-top-width", @"border-right-width", @"border-bottom-width"
         ]) {
      [LynxPropsProcessor updateProp:@60 withKey:key forUI:_view];
    }
    [_view propsDidUpdate];
    [_view onNodeReadyForUIOwner];
    XCTAssertNotEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
    XCTAssertEqual(_view.backgroundManager.borderLayer.borderWidth, 0);
    XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 0);
    XCTAssertEqual(_view.view.layer.cornerRadius, 10);
  }
}

- (void)testResolvedPartialRadiiAndResizeFallback {
  if (@available(iOS 11.0, *)) {
    [self layoutWithSize:CGSizeMake(120, 120)];
    [LynxPropsProcessor updateProp:@(LynxOverflowHidden) withKey:@"overflow" forUI:_view];
    [self setCornerValues:@[
      @[ @0, @0, @0, @0 ], @[ @0.1, @1, @0.1, @1 ], @[ @0, @0, @0, @0 ], @[ @0, @0, @0, @0 ]
    ]];
    [self layoutWithSize:CGSizeMake(100, 100)];
    XCTAssertEqual(_view.view.layer.cornerRadius, 10);
    XCTAssertNil(_view.view.layer.mask);
    [self layoutWithSize:CGSizeMake(200, 100)];
    // Percentages resolve to an ellipse in a non-square reference box.
    XCTAssertEqual(_view.backgroundManager.borderRadius.topRightX.val, 20);
    XCTAssertEqual(_view.backgroundManager.borderRadius.topRightY.val, 10);
    XCTAssertEqual(_view.view.layer.cornerRadius, 0);
    XCTAssertNotNil(_view.view.layer.mask);
    [self setCornerSubset:2 radius:@60];
    XCTAssertNotNil(_view.view.layer.mask);
    [self layoutWithSize:CGSizeMake(200, 200)];
    XCTAssertEqual(_view.view.layer.cornerRadius, 60);
    XCTAssertNil(_view.view.layer.mask);
    XCTAssertNil(_view.backgroundManager.borderLayer);
  }
}

- (void)testPartialCornersDoNotBroadenBorderStyleWidthOrColorEligibility {
  if (@available(iOS 11.0, *)) {
    NSArray* keys = @[ @"border-left-width", @"border-left-style", @"border-left-color" ];
    NSArray* values = @[ @2, @(LynxBorderStyleDashed), @0xFFFF0000 ];
    NSArray* defaults = @[ @1, @(LynxBorderStyleSolid), @0xFF000000 ];
    for (NSUInteger i = 0; i < keys.count; ++i) {
      [self setCornerSubset:2 radius:@10];
      XCTAssertEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
      [LynxPropsProcessor updateProp:values[i] withKey:keys[i] forUI:_view];
      [_view propsDidUpdate];
      [_view onNodeReadyForUIOwner];
      XCTAssertNotEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
      XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 0);
      [LynxPropsProcessor updateProp:defaults[i] withKey:keys[i] forUI:_view];
      [_view propsDidUpdate];
      [_view onNodeReadyForUIOwner];
    }
  }
}

- (void)testPartialCornersKeepComplexBackgroundExclusions {
  if (@available(iOS 11.0, *)) {
    [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];
    [self setCornerSubset:2 radius:@10];
    [LynxPropsProcessor updateProp:@[ @(LynxBackgroundClipContentBox) ]
                           withKey:@"background-clip"
                             forUI:_view];
    [_view propsDidUpdate];
    [_view onNodeReadyForUIOwner];
    XCTAssertEqual(_view.backgroundManager.backgroundLayer.type, LynxBgTypeComplex);
    XCTAssertEqual(_view.backgroundManager.backgroundLayer.cornerRadius, 0);
    [LynxPropsProcessor updateProp:nil withKey:@"background-clip" forUI:_view];
    [LynxPropsProcessor updateProp:@[ @[ @0, @0, @5, @0, @1, @0xFF000000 ] ]
                           withKey:@"box-shadow"
                             forUI:_view];
    [_view propsDidUpdate];
    [_view onNodeReadyForUIOwner];
    XCTAssertEqual(_view.backgroundManager.backgroundLayer.type, LynxBgTypeComplex);
    XCTAssertFalse(_view.backgroundManager.backgroundLayer.masksToBounds);
    XCTAssertEqual(_view.backgroundManager.backgroundLayer.cornerRadius, 0);
  }
}

- (void)testPartialRadiiUseNormalizedUntransformedBounds {
  if (@available(iOS 11.0, *)) {
    [self setCornerSubset:3 radius:@75];
    [self layoutWithSize:CGSizeMake(100, 200)];
    XCTAssertEqual(_view.backgroundManager.borderRadius.topLeftX.val, 50);
    XCTAssertEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
    XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 50);
    [self setCornerSubset:2 radius:@10];
    _view.view.layer.transform = CATransform3DMakeScale(0.1, 0.1, 1);
    [_view.backgroundManager applyEffect:YES];
    XCTAssertEqual(_view.view.layer.cornerRadius, 10);
    XCTAssertEqual(_view.backgroundManager.borderLayer.cornerRadius, 10);
    XCTAssertEqual(_view.backgroundManager.borderLayer.maskedCorners, kCALayerMaxXMinYCorner);
    [self setCornerValues:@[
      @[ @0, @0, @0, @0 ], @[ @[ @0.1, @1, @10, @0 ], @2, @[ @0.1, @1, @10, @0 ], @2 ],
      @[ @0, @0, @0, @0 ], @[ @0, @0, @0, @0 ]
    ]];
    [self layoutWithSize:CGSizeMake(100, 100)];
    XCTAssertEqual(_view.view.layer.cornerRadius, 20);
    XCTAssertEqual(_view.view.layer.maskedCorners, kCALayerMaxXMinYCorner);
  }
}

- (void)testZeroAxisRadiusOverlapKeepsPathGeometry {
  if (@available(iOS 11.0, *)) {
    [LynxPropsProcessor updateProp:@(LynxOverflowHidden) withKey:@"overflow" forUI:_view];
    [self setCornerValues:@[
      @[ @0, @0, @200, @0 ], @[ @10, @0, @10, @0 ], @[ @0, @0, @0, @0 ], @[ @0, @0, @0, @0 ]
    ]];
    XCTAssertEqual(_view.view.layer.cornerRadius, 0);
    XCTAssertFalse(_view.view.clipsToBounds);
    CALayer* mask = _view.view.layer.mask;
    XCTAssertTrue([mask isKindOfClass:CAShapeLayer.class]);
    if ([mask isKindOfClass:CAShapeLayer.class]) {
      XCTAssertTrue(CGPathContainsPoint(((CAShapeLayer*)mask).path, NULL, CGPointMake(96, 1), NO));
    }
    [self layoutWithSize:CGSizeMake(120, 120)];
    [self layoutWithSize:CGSizeMake(100, 100)];
    XCTAssertEqual(_view.view.layer.cornerRadius, 5);
    XCTAssertEqual(_view.view.layer.maskedCorners, kCALayerMaxXMinYCorner);
    XCTAssertNil(_view.view.layer.mask);
  }
}

- (void)testExternalMaskKeepsPartialPaintingOnSiblings {
  if (@available(iOS 11.0, *)) {
    [LynxPropsProcessor updateProp:@(LynxOverflowHidden) withKey:@"overflow" forUI:_view];
    [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];
    [self setCornerSubset:2 radius:@10];
    CALayer* mask = [CALayer layer];
    _view.view.layer.mask = mask;
    [_view.backgroundManager applyEffect];
    XCTAssertEqual(_view.view.layer.mask, mask);
    XCTAssertEqual(_view.view.layer.cornerRadius, 0);
    XCTAssertEqual(_view.view.layer.borderWidth, 0);
    XCTAssertNotEqual(_view.backgroundManager.borderLayer.type, LynxBgTypeSimple);
    XCTAssertEqual(_view.backgroundManager.backgroundLayer.type, LynxBgTypeComplex);
    XCTAssertNil(_view.backgroundManager.borderLayer.mask);
    XCTAssertNil(_view.backgroundManager.backgroundLayer.mask);
    _view.view.layer.mask = nil;
    [_view.backgroundManager applyEffect];
    XCTAssertNil(_view.backgroundManager.borderLayer);
    XCTAssertNil(_view.backgroundManager.backgroundLayer);
    XCTAssertEqual(_view.view.layer.cornerRadius, 10);
  }
}

- (void)testPartialPaintingMigratesWhenOnlyOverflowChanges {
  if (@available(iOS 11.0, *)) {
    [LynxPropsProcessor updateProp:@0xFFFF0000 withKey:@"background-color" forUI:_view];
    [self setCornerSubset:2 radius:@10];
    for (NSNumber* overflow in
         @[ @(LynxOverflowHidden), @(LynxOverflowVisible), @(LynxOverflowHidden) ]) {
      [LynxPropsProcessor updateProp:overflow withKey:@"overflow" forUI:_view];
      [_view propsDidUpdate];
      [_view onNodeReadyForUIOwner];
      BOOL hidden = overflow.intValue == LynxOverflowHidden;
      XCTAssertEqual(_view.backgroundManager.borderLayer == nil, hidden);
      XCTAssertEqual(_view.backgroundManager.backgroundLayer == nil, hidden);
      XCTAssertEqual(_view.view.layer.borderWidth, hidden ? 1 : 0);
      XCTAssertEqual(_view.view.layer.maskedCorners, kCALayerMaxXMinYCorner);
    }
  }
}

@end
