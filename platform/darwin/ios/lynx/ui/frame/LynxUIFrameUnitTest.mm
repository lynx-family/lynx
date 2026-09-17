// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <Lynx/LynxBackgroundManager.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxUIFrame.h>
#import <XCTest/XCTest.h>

#include "base/include/value/table.h"

@interface LynxUIFrameUnitTest : XCTestCase
@end

@implementation LynxUIFrameUnitTest

- (LynxTemplateData *)pendingInitDataForUIFrame:(LynxUIFrame *)uiFrame {
  return [uiFrame.view valueForKey:@"initData"];
}

- (LynxUIFrame *)frameWithBorder {
  LynxUIFrame *frameUI = [[LynxUIFrame alloc] initWithView:nil];
  for (NSString *edge in @[ @"left", @"top", @"right", @"bottom" ]) {
    [LynxPropsProcessor updateProp:@3
                           withKey:[NSString stringWithFormat:@"border-%@-width", edge]
                             forUI:frameUI];
    [LynxPropsProcessor updateProp:@0xFFFF0000
                           withKey:[NSString stringWithFormat:@"border-%@-color", edge]
                             forUI:frameUI];
  }
  return frameUI;
}

- (void)assertChildPageLayoutPreservesFrameBorderWithW3CDefaults:(BOOL)useW3CDefaults {
  LynxUIFrame *frameUI = [self frameWithBorder];
  [frameUI updateFrame:CGRectMake(0, 0, 200, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsMake(3, 3, 3, 3)
                   margin:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  XCTAssertEqual(frameUI.view.layer.borderWidth, 3);

  LynxRootUI *childPage = [[LynxRootUI alloc] initWithLynxView:frameUI.view];
  XCTAssertEqual(childPage.view, frameUI.view);
  if (useW3CDefaults) {
    [childPage.backgroundManager makeCssDefaultValueToFitW3c];
  }
  XCTAssertEqual([childPage.backgroundManager.backgroundInfo hasBorder], useW3CDefaults);
  XCTAssertFalse(childPage.backgroundManager.backgroundInfo.borderChanged);
  [childPage propsDidUpdate];
  XCTAssertEqual(frameUI.view.layer.borderWidth, 3);

  for (NSNumber *width in @[ @194, @294 ]) {
    [childPage updateFrame:CGRectMake(0, 0, width.doubleValue, 94)
                withPadding:UIEdgeInsetsZero
                     border:UIEdgeInsetsZero
                     margin:UIEdgeInsetsZero
        withLayoutAnimation:NO];
    XCTAssertEqual(frameUI.view.layer.borderWidth, 3);
    XCTAssertTrue(CGColorEqualToColor(frameUI.view.layer.borderColor, UIColor.redColor.CGColor));
  }

  for (NSString *edge in @[ @"left", @"top", @"right", @"bottom" ]) {
    [LynxPropsProcessor updateProp:@0
                           withKey:[NSString stringWithFormat:@"border-%@-width", edge]
                             forUI:frameUI];
  }
  [frameUI.backgroundManager applyEffect];
  XCTAssertEqual(frameUI.view.layer.borderWidth, 0);
}

- (void)testChildPageLayoutPreservesFrameBorder {
  [self assertChildPageLayoutPreservesFrameBorderWithW3CDefaults:NO];
}

- (void)testBorderlessW3CChildPageLayoutPreservesFrameBorder {
  [self assertChildPageLayoutPreservesFrameBorderWithW3CDefaults:YES];
}

- (void)testChildPageLayoutPreservesPartialFrameCorners {
  LynxUIFrame *frameUI = [self frameWithBorder];
  [LynxPropsProcessor updateProp:@YES withKey:@"background-shape-layer" forUI:frameUI];
  for (NSString *key in @[ @"border-top-left-radius", @"border-top-right-radius" ]) {
    [LynxPropsProcessor updateProp:@[ @30, @0, @30, @0 ] withKey:key forUI:frameUI];
  }
  [frameUI updateFrame:CGRectMake(0, 0, 200, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsMake(3, 3, 3, 3)
                   margin:UIEdgeInsetsZero
      withLayoutAnimation:NO];

  BOOL (^hasRoundedBorderAndClip)(void) = ^BOOL(void) {
    CALayer *layer = frameUI.view.layer;
    if (@available(iOS 11.0, *)) {
      if (layer.borderWidth == 3 && layer.cornerRadius == 30 && layer.masksToBounds &&
          layer.maskedCorners == (kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner)) {
        return YES;
      }
    }
    LynxBorderLayer *border = frameUI.backgroundManager.borderLayer;
    if (border.type != LynxBgTypeShape || !border.path || border.lineWidth != 3 ||
        ![layer.mask isKindOfClass:CAShapeLayer.class]) {
      return NO;
    }
    CGPathRef clip = ((CAShapeLayer *)layer.mask).path;
    return clip && !CGPathContainsPoint(border.path, NULL, CGPointMake(2, 2), NO) &&
           CGPathContainsPoint(border.path, NULL, CGPointMake(100, 50), NO) &&
           !CGPathContainsPoint(clip, NULL, CGPointMake(2, 2), NO) &&
           CGPathContainsPoint(clip, NULL, CGPointMake(100, 50), NO);
  };
  XCTAssertTrue(hasRoundedBorderAndClip());

  LynxRootUI *childPage = [[LynxRootUI alloc] initWithLynxView:frameUI.view];
  [childPage propsDidUpdate];
  XCTAssertTrue(hasRoundedBorderAndClip());
  [childPage updateFrame:CGRectMake(0, 0, 194, 94)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
                   margin:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  XCTAssertTrue(hasRoundedBorderAndClip());
}

- (void)testDataDictionaryStoresPendingTemplateData {
  LynxUIFrame *uiFrame = [[LynxUIFrame alloc] initWithView:nil];

  [LynxPropsProcessor updateProp:@{@"value" : @2} withKey:@"data" forUI:uiFrame];

  LynxTemplateData *pendingData = [self pendingInitDataForUIFrame:uiFrame];
  XCTAssertNotNil(pendingData);
  XCTAssertEqual(2, [[pendingData dictionary][@"value"] intValue]);
}

- (void)testDataPointerStoresPendingTemplateData {
  LynxUIFrame *uiFrame = [[LynxUIFrame alloc] initWithView:nil];
  auto table = lynx::lepus::Dictionary::Create();
  table->SetValue("value", lynx::lepus::Value(3));
  auto *value = new lynx::lepus::Value(table);
  auto ptr = reinterpret_cast<NSInteger>(value);
  XCTAssertNotEqual(ptr, 0);

  [LynxPropsProcessor updateProp:@(ptr) withKey:@"data" forUI:uiFrame];

  LynxTemplateData *pendingData = [self pendingInitDataForUIFrame:uiFrame];
  XCTAssertNotNil(pendingData);
  XCTAssertEqual(3, [[pendingData dictionary][@"value"] intValue]);
}

@end
