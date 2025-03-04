// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <XCTest/XCTest.h>
#import "LynxPropsProcessor.h"
#import "LynxUI+Private.h"
#import "LynxUI.h"
#import "LynxUIImage.h"
#import "LynxUIMethodProcessor.h"
#import "LynxUIOwner+Accessibility.h"
#import "LynxUIOwner.h"
#import "LynxUIView.h"
#import "LynxView.h"

#define LYNX_A11Y_TAG_TESTING(pointer) ((NSInteger)pointer + 11)

@interface LynxUIOwner (TestAccessibility)
@property(nonatomic) NSMutableArray *a11yMutationList;
@end

@interface LynxUIOwnerAccessibilityUnitTest : XCTestCase
@property(nonatomic, strong) LynxUIOwner *uiOwner;
@end

@implementation LynxUIOwnerAccessibilityUnitTest

- (void)setUp {
  self.uiOwner = [[LynxUIOwner alloc] init];
  self.uiOwner.a11yMutationList = [[NSMutableArray alloc] init];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testMutations {
  LynxUI *child = [[LynxUIImage alloc] init];
  child.sign = 1;

  [self.uiOwner addA11yMutation:@"insert"
                           sign:@(child.sign)
                         a11yID:child.a11yID
                        toArray:self.uiOwner.a11yMutationList];

  child.sign = 2;
  child.a11yID = @"2";
  [self.uiOwner addA11yMutation:@"remove"
                           sign:@(child.sign)
                         a11yID:child.a11yID
                        toArray:self.uiOwner.a11yMutationList];

  child.sign = 3;
  child.a11yID = @"3";
  [self.uiOwner addA11yMutation:@"update"
                           sign:@(child.sign)
                         a11yID:child.a11yID
                        toArray:self.uiOwner.a11yMutationList];

  child.sign = 4;
  child.a11yID = @"4";
  [self.uiOwner addA11yMutation:@"detach"
                           sign:@(child.sign)
                         a11yID:child.a11yID
                        toArray:self.uiOwner.a11yMutationList];

  XCTAssert([@"insert" isEqualToString:[self.uiOwner.a11yMutationList[0] objectForKey:@"action"]]);
  XCTAssert([@"remove" isEqualToString:[self.uiOwner.a11yMutationList[1] objectForKey:@"action"]]);
  XCTAssert([@"update" isEqualToString:[self.uiOwner.a11yMutationList[2] objectForKey:@"action"]]);
  XCTAssert([@"detach" isEqualToString:[self.uiOwner.a11yMutationList[3] objectForKey:@"action"]]);

  XCTAssert([@"" isEqualToString:[self.uiOwner.a11yMutationList[0] objectForKey:@"a11y-id"]]);
  XCTAssert([@"2" isEqualToString:[self.uiOwner.a11yMutationList[1] objectForKey:@"a11y-id"]]);
  XCTAssert([@"3" isEqualToString:[self.uiOwner.a11yMutationList[2] objectForKey:@"a11y-id"]]);
  XCTAssert([@"4" isEqualToString:[self.uiOwner.a11yMutationList[3] objectForKey:@"a11y-id"]]);

  XCTAssert([@(1) isEqualToNumber:[self.uiOwner.a11yMutationList[0] objectForKey:@"target"]]);
  XCTAssert([@(2) isEqualToNumber:[self.uiOwner.a11yMutationList[1] objectForKey:@"target"]]);
  XCTAssert([@(3) isEqualToNumber:[self.uiOwner.a11yMutationList[2] objectForKey:@"target"]]);
  XCTAssert([@(4) isEqualToNumber:[self.uiOwner.a11yMutationList[3] objectForKey:@"target"]]);

  // we must not creat LynxView
  [self.uiOwner flushMutations:self.uiOwner.a11yMutationList withBodyView:nil];
  XCTAssert(self.uiOwner.a11yMutationList.count == 0);
}

- (void)testPropMutations {
  [self.uiOwner setA11yFilter:[NSSet setWithArray:@[ @"background", @"color" ]]];
  [self.uiOwner addA11yPropsMutation:@"abc"
                                sign:@(1)
                              a11yID:@"1"
                             toArray:self.uiOwner.a11yMutationList];
  [self.uiOwner addA11yPropsMutation:@"background"
                                sign:@(1)
                              a11yID:@"1"
                             toArray:self.uiOwner.a11yMutationList];
  [self.uiOwner addA11yPropsMutation:@"color"
                                sign:@(1)
                              a11yID:@"1"
                             toArray:self.uiOwner.a11yMutationList];
  XCTAssert([@(1) isEqualToNumber:[self.uiOwner.a11yMutationList[0] objectForKey:@"target"]]);
  XCTAssert(
      [@"style_update" isEqualToString:[self.uiOwner.a11yMutationList[0] objectForKey:@"action"]]);
  XCTAssert(
      [@"background" isEqualToString:[self.uiOwner.a11yMutationList[0] objectForKey:@"style"]]);

  XCTAssert([@(1) isEqualToNumber:[self.uiOwner.a11yMutationList[1] objectForKey:@"target"]]);
  XCTAssert(
      [@"style_update" isEqualToString:[self.uiOwner.a11yMutationList[1] objectForKey:@"action"]]);
  XCTAssert([@"color" isEqualToString:[self.uiOwner.a11yMutationList[1] objectForKey:@"style"]]);

  [self.uiOwner flushMutations:self.uiOwner.a11yMutationList withBodyView:nil];
  XCTAssert(self.uiOwner.a11yMutationList.count == 0);
}

- (void)testNestedA11yElements {
  LynxUIView *parent = [[LynxUIView alloc] init];
  LynxUIView *child1 = [[LynxUIView alloc] init];
  LynxUIView *child2 = [[LynxUIView alloc] init];

  [parent insertChild:child1 atIndex:0];
  [parent insertChild:child2 atIndex:1];

  [LynxPropsProcessor updateProp:@(YES) withKey:@"accessibility-element" forUI:parent];
  [LynxPropsProcessor updateProp:@(YES) withKey:@"accessibility-element" forUI:child1];
  [LynxPropsProcessor updateProp:@(YES) withKey:@"accessibility-element" forUI:child2];

  [parent onNodeReadyForUIOwner];
  [child1 onNodeReadyForUIOwner];
  [child2 onNodeReadyForUIOwner];

  [self.uiOwner checkNestedAccessibilityElements:parent];

  XCTAssert(!parent.view.isAccessibilityElement);
  XCTAssert(child1.view.isAccessibilityElement);
  XCTAssert(child2.view.isAccessibilityElement);

  UIView *dummyView = parent.view.subviews.firstObject;

  XCTAssert(dummyView.tag == LYNX_A11Y_TAG_TESTING(parent));
  XCTAssert(dummyView.isAccessibilityElement);
}

@end
