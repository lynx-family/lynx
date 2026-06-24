// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXView.h>
#import <XElement/LynxAnimaXContainerView.h>

#import <XCTest/XCTest.h>

@interface LynxAnimaXContainerViewTests : XCTestCase
@end

@implementation LynxAnimaXContainerViewTests

- (void)testPaddingAffectsSubviewFrame {
  CGRect containerFrame = CGRectMake(0, 0, 200, 100);
  LynxAnimaXContainerView *container = [[LynxAnimaXContainerView alloc] init];

  BaseAnimaXAbility *ability = [[BaseAnimaXAbility alloc] init];
  AnimaXContext *ctx = [[AnimaXContext alloc] initWithAbility:ability];
  AnimaXView *view = [[AnimaXView alloc] initWithContext:ctx];
  [container setContentView:view];

  container.frame = containerFrame;
  [container setNeedsLayout];
  [container layoutIfNeeded];
  XCTAssertTrue(CGRectEqualToRect(view.frame, view.bounds));

  UIEdgeInsets padding = UIEdgeInsetsMake(10, 20, 30, 40);
  [container setPadding:padding];
  [container layoutIfNeeded];
  CGRect expected = UIEdgeInsetsInsetRect(container.bounds, padding);
  XCTAssertEqualWithAccuracy(CGRectGetMinX(view.frame), CGRectGetMinX(expected), 0.5);
  XCTAssertEqualWithAccuracy(CGRectGetMinY(view.frame), CGRectGetMinY(expected), 0.5);
  XCTAssertEqualWithAccuracy(CGRectGetWidth(view.frame), CGRectGetWidth(expected), 0.5);
  XCTAssertEqualWithAccuracy(CGRectGetHeight(view.frame), CGRectGetHeight(expected), 0.5);
}

@end
