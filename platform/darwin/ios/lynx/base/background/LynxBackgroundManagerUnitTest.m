// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundManager.h>
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

@end
