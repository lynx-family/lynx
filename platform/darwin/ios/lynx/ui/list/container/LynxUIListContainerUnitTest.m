// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIListContainer.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxVersion.h>
#import <XCTest/XCTest.h>
#import "LynxListStickyManager.h"
#import "LynxUI+Gesture.h"

@interface LynxUIListContainer (Testing)
@property(nonatomic, assign) CGFloat pagingAlignFactor;
@property(nonatomic, assign) CGFloat pagingAlignOffset;
@property(nonatomic, strong) LynxListStickyManager *stickyManager;
@end

@interface LynxListUIContainerUnitTest : XCTestCase

@end
@implementation LynxListUIContainerUnitTest

- (void)setUp {
}

- (LynxUIListContainer *)setUpList {
  LynxUIListContainer *list = [[LynxUIListContainer alloc] init];
  [list updateFrame:UIScreen.mainScreen.bounds
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [list.view setContentSize:CGSizeMake(UIScreen.mainScreen.bounds.size.width,
                                       UIScreen.mainScreen.bounds.size.height * 5)];
  return list;
}

- (void)testSnap {
  LynxUIListContainer *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [LynxPropsProcessor updateProp:@{
    @"factor" : @(0),
    @"offset" : @(20),
  }
                         withKey:@"item-snap"
                           forUI:list];
  XCTAssertTrue(list.pagingAlignFactor == 0);
  XCTAssertTrue(list.pagingAlignOffset == 20);
  [LynxPropsProcessor updateProp:@{
    @"factor" : @(-1),
    @"align" : @(20),
  }
                         withKey:@"item-snap"
                           forUI:list];
}

- (void)testStickyPropertiesAreForwardedToManager {
  LynxUIListContainer *list = [self setUpList];
  XCTAssertNotNil(list.stickyManager);

  [LynxPropsProcessor updateProp:@YES withKey:@"sticky" forUI:list];
  [LynxPropsProcessor updateProp:@12.5 withKey:@"sticky-offset" forUI:list];

  XCTAssertTrue(list.stickyManager.enabled);
  XCTAssertEqualWithAccuracy(list.stickyManager.offset, 12.5, 0.001);
}

@end
