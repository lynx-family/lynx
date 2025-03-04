// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <XCTest/XCTest.h>
#import "LynxPropsProcessor.h"
#import "LynxTextRenderer.h"
#import "LynxUI+Private.h"
#import "LynxUI.h"
#import "LynxUIImage.h"
#import "LynxUIMethodProcessor.h"
#import "LynxUIOwner+Accessibility.h"
#import "LynxUIOwner.h"
#import "LynxUIText.h"
#import "LynxUIUnitTestUtils.h"
#import "LynxUIView.h"
#import "LynxView.h"

@interface LynxUI (TestAccessibility)
- (UIView *)accessibilityFocusedView;
- (NSArray *)accessibilityElementsWithA11yID;
- (void)requestAccessibilityFocus:(NSDictionary *)params
                       withResult:(LynxUIMethodCallbackBlock)callback;
- (void)fetchAccessibilityTargets:(NSDictionary *)params
                       withResult:(LynxUIMethodCallbackBlock)callback;
- (void)innerText:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
@end

@interface LynxUI_AccessibilityUnitTest : XCTestCase

@end

@implementation LynxUI_AccessibilityUnitTest

- (void)setUp {
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testRequestAccessibilityFocus {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils updateUIMockContext:nil
                                          sign:1
                                           tag:@"view"
                                      eventSet:[NSSet set]
                                 lepusEventSet:[NSSet set]
                                         props:[NSDictionary dictionary]];
  LynxUI *ui = [mockContext.UIOwner findUIBySign:1];
  [ui requestAccessibilityFocus:@{@"withoutUpdate" : @(YES)}
                     withResult:^(int code, id _Nullable data) {
                       XCTAssert(code == 0);
                     }];
  [ui requestAccessibilityFocus:nil
                     withResult:^(int code, id _Nullable data) {
                       XCTAssert(code == 0);
                     }];
  XCTAssert([ui accessibilityFocusedView] == ui.view);
}

- (void)testFetchAccessibilityTargets {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils updateUIMockContext:nil
                                          sign:1
                                           tag:@"view"
                                      eventSet:[NSSet set]
                                 lepusEventSet:[NSSet set]
                                         props:[NSDictionary dictionary]];
  LynxUI *ui = [mockContext.UIOwner findUIBySign:1];
  [ui fetchAccessibilityTargets:nil
                     withResult:^(int code, NSArray *_Nullable data) {
                       XCTAssert(code == 0);
                       XCTAssert([@"unknown" isEqualToString:[data[0] objectForKey:@"a11y-id"]]);
                       XCTAssert([@(1) isEqualToNumber:[data[0] objectForKey:@"element-id"]]);
                     }];
  LynxUI *child = [[LynxUI alloc] initWithView:[[UIView alloc] init]];
  child.sign = 22;
  child.a11yID = @"22";
  [ui insertChild:child atIndex:0];
  [ui fetchAccessibilityTargets:nil
                     withResult:^(int code, NSArray *_Nullable data) {
                       XCTAssert(code == 0);
                       XCTAssert([@"22" isEqualToString:[data[1] objectForKey:@"a11y-id"]]);
                       XCTAssert([@(22) isEqualToNumber:[data[1] objectForKey:@"element-id"]]);
                     }];
}

- (void)testInnerText {
  LynxUI *ui = [[LynxUI alloc] init];
  LynxUIText *child = [[LynxUIText alloc] init];
  LynxLayoutSpec *spec = [[LynxLayoutSpec alloc] init];
  [child onReceiveUIOperation:[[LynxTextRenderer alloc]
                                  initWithAttributedString:[[NSAttributedString alloc]
                                                               initWithString:@"hello"]
                                                layoutSpec:spec]];
  [ui insertChild:child atIndex:0];

  LynxUIText *child2 = [[LynxUIText alloc] init];
  [child2 onReceiveUIOperation:[[LynxTextRenderer alloc]
                                   initWithAttributedString:[[NSAttributedString alloc]
                                                                initWithString:@"lynx"]
                                                 layoutSpec:spec]];
  [ui insertChild:child2 atIndex:1];
  [ui innerText:nil
      withResult:^(int code, id _Nullable data) {
        XCTAssert(code == 0);
        XCTAssert([@"hello" isEqualToString:data[0]]);
        XCTAssert([@"lynx" isEqualToString:data[1]]);
      }];
}

- (void)testAccessibilityElementsWithA11yID {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils updateUIMockContext:nil
                                          sign:0
                                           tag:@"view"
                                      eventSet:[NSSet set]
                                 lepusEventSet:[NSSet set]
                                         props:[NSDictionary dictionary]];
  LynxUI *ui = [mockContext.UIOwner findUIBySign:0];

  mockContext = [LynxUIUnitTestUtils updateUIMockContext:mockContext
                                                    sign:11
                                                     tag:@"view"
                                                eventSet:[NSSet set]
                                           lepusEventSet:[NSSet set]
                                                   props:@{@"a11y-id" : @"a11y_11"}];
  LynxUI *child1 = [mockContext.UIOwner findUIBySign:11];
  [ui insertChild:child1 atIndex:0];

  mockContext = [LynxUIUnitTestUtils updateUIMockContext:mockContext
                                                    sign:111
                                                     tag:@"view"
                                                eventSet:[NSSet set]
                                           lepusEventSet:[NSSet set]
                                                   props:@{@"a11y-id" : @"a11y_same"}];
  LynxUI *child11 = [mockContext.UIOwner findUIBySign:111];
  [child11 updateFrame:CGRectMake(1, 0, 1, 5)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [child1 insertChild:child11 atIndex:0];

  mockContext = [LynxUIUnitTestUtils updateUIMockContext:mockContext
                                                    sign:12
                                                     tag:@"view"
                                                eventSet:[NSSet set]
                                           lepusEventSet:[NSSet set]
                                                   props:@{@"a11y-id" : @"a11y_12"}];
  LynxUI *child2 = [mockContext.UIOwner findUIBySign:12];
  [child2 updateFrame:CGRectMake(2, 0, 1, 3)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [ui insertChild:child2 atIndex:1];

  mockContext = [LynxUIUnitTestUtils updateUIMockContext:mockContext
                                                    sign:121
                                                     tag:@"view"
                                                eventSet:[NSSet set]
                                           lepusEventSet:[NSSet set]
                                                   props:@{@"a11y-id" : @"a11y_same"}];
  LynxUI *child12 = [mockContext.UIOwner findUIBySign:121];
  [child12 updateFrame:CGRectMake(3, 0, 1, 2)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [child2 insertChild:child12 atIndex:0];

  mockContext = [LynxUIUnitTestUtils updateUIMockContext:mockContext
                                                    sign:233
                                                     tag:@"view"
                                                eventSet:[NSSet set]
                                           lepusEventSet:[NSSet set]
                                                   props:@{@"a11y-id" : @"a11y_same"}];
  LynxUI *childSpecial = [mockContext.UIOwner findUIBySign:233];
  [childSpecial updateFrame:CGRectMake(4, 0, 1, 4)
                withPadding:UIEdgeInsetsZero
                     border:UIEdgeInsetsZero
        withLayoutAnimation:NO];
  [ui insertChild:childSpecial atIndex:0];

  [LynxPropsProcessor updateProp:@"a11y_same,12,11,233"
                         withKey:@"accessibility-elements-a11y"
                           forUI:ui];

  NSArray *array = [ui accessibilityElementsWithA11yID];
  XCTAssert(array[0] == child11.view);
  XCTAssert(array[1] == child12.view);
  XCTAssert(array[2] == child2.view);
  XCTAssert(array[3] == child1.view);
  XCTAssert(array[4] == childSpecial.view);
}

- (void)disable_testExclusiveA11yElements {
  LynxUIView *parent = [[LynxUIView alloc] init];
  LynxUIView *child1 = [[LynxUIView alloc] init];
  LynxUIView *child2 = [[LynxUIView alloc] init];
  LynxUIView *child3 = [[LynxUIView alloc] init];

  [parent insertChild:child1 atIndex:0];
  [parent insertChild:child2 atIndex:1];
  [child2 insertChild:child3 atIndex:0];

  [LynxPropsProcessor updateProp:@(YES) withKey:@"accessibility-element" forUI:child1];
  [LynxPropsProcessor updateProp:@(YES) withKey:@"accessibility-element" forUI:child3];

  [LynxPropsProcessor updateProp:@(YES) withKey:@"accessibility-exclusive-focus" forUI:child2];
  [child2 onNodeReadyForUIOwner];

  XCTAssert(child1.view.accessibilityElementsHidden);

  [LynxPropsProcessor updateProp:@(NO) withKey:@"accessibility-exclusive-focus" forUI:child2];

  [child2 onNodeReadyForUIOwner];

  XCTAssert(!child1.view.accessibilityElementsHidden);
}

@end
