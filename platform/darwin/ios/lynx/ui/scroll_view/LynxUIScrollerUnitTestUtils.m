// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIScrollerUnitTestUtils.h"
#import <Lynx/LynxBounceView.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUIScroller.h>
#import <XCTest/XCTest.h>
#import "LynxUI+Private.h"
#import "LynxUIUnitTestUtils.h"

@implementation LynxUIScrollerUnitTestUtils
#pragma mark mock utils

+ (void)mockBounceView:(LynxUIMockContext *)context
                direction:(NSString *)direction
    triggerBounceDistance:(CGFloat)distance
                     size:(CGSize)size {
  LynxBounceView *bounceView =
      [[LynxBounceView alloc] initWithView:[[UIView alloc] initWithFrame:(CGRect){{0, 0}, size}]];
  [LynxPropsProcessor updateProp:direction withKey:@"direction" forUI:bounceView];
  bounceView.triggerBounceEventDistance = distance;
  [bounceView propsDidUpdate];
  [context.mockUI insertChild:bounceView atIndex:context.mockUI.children.count];
  [context.mockUI layoutDidFinished];
}

+ (void)mockChildren:(NSInteger)count
             context:(LynxUIMockContext *)context
             scrollY:(BOOL)enableScrollY
                size:(CGSize)size {
  for (int i = 0; i < count; i++) {
    LynxUI *child = [[LynxUIView alloc] init];
    CGRect childFrame;
    if (enableScrollY) {
      childFrame = (CGRect){{0, size.height * i + context.mockUI.padding.top}, size};
    } else {
      childFrame = (CGRect){{size.width * i + context.mockUI.padding.left, 0}, size};
    }
    [child updateFrame:childFrame
                withPadding:UIEdgeInsetsZero
                     border:UIEdgeInsetsZero
                     margin:UIEdgeInsetsZero
        withLayoutAnimation:false];
    [context.mockUI insertChild:child atIndex:i];
  }
  [context.mockUI layoutDidFinished];

  UIScrollView *scrollView = (UIScrollView *)context.mockUI.view;
  if (enableScrollY) {
    XCTAssertEqual(scrollView.contentSize.height, count * size.height + context.mockUI.padding.top +
                                                      context.mockUI.padding.bottom);
    XCTAssertEqual(scrollView.contentSize.width, MAX(size.width, context.mockUI.frame.size.width));
  } else {
    XCTAssertEqual(scrollView.contentSize.width,
                   count * size.width + context.mockUI.padding.left + context.mockUI.padding.right);
    XCTAssertEqual(scrollView.contentSize.height,
                   MAX(size.height, context.mockUI.frame.size.height));
  }
}

+ (LynxUIMockContext *)updateUIMockContext:(LynxUIMockContext *)mockContext
                                      sign:(NSInteger)sign
                                       tag:(NSString *)tagName
                                  eventSet:(NSSet *)eventSet
                             lepusEventSet:(NSSet *)lepusEventSet
                                     props:(NSDictionary *)props {
  mockContext = [super updateUIMockContext:mockContext
                                      sign:sign
                                       tag:tagName
                                  eventSet:eventSet
                             lepusEventSet:lepusEventSet
                                     props:props];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  mockContext.mockUI = scroller;
  [scroller updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:scroller.enableScrollY
                                       size:CGSizeMake(100.0f, 100.0f)];
  [scroller onNodeReadyForUIOwner];
  return mockContext;
}

@end
