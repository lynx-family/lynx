// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <XCTest/XCTest.h>
#import "LynxUIMethodProcessor.h"
#import "LynxUIScroller.h"
#import "LynxUIScrollerUnitTestUtils.h"

@interface LynxUIScrollerUIMethodScrollToUnitTest : XCTestCase
@end

@implementation LynxUIScrollerUIMethodScrollToUnitTest
#pragma mark scrollTo - offset
- (void)testScrollToOffset0 {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:0]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, 0);
}

- (void)testScrollToOffsetNegative {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:-100],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:0]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, 0);
}

- (void)testScrollToOffsetMaxValue {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInteger:NSIntegerMax],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:0]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentOffset.x,
                 scrollView.contentSize.width - scrollView.frame.size.width);
}

- (void)testScrollToOffsetRandom {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  float targetOffset = 456;
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInteger:targetOffset],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:0]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, targetOffset);
}

- (void)testScrollToOffsetEnd {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor
      invokeMethod:@"scrollTo"
        withParams:@{
          @"offset" : [NSNumber
              numberWithInteger:((UIScrollView *)mockContext.mockUI.view).contentSize.width -
                                ((UIScrollView *)mockContext.mockUI.view).frame.size.width + 1],
          @"smooth" : [NSNumber numberWithBool:0],
          @"index" : [NSNumber numberWithInt:0]
        }
        withResult:callBack
             forUI:mockContext.mockUI];

  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentOffset.x,
                 scrollView.contentSize.width - scrollView.frame.size.width);
}

#pragma mark scrollTo - index
- (void)testScrollToIndex0 {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:0]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, 0);
}

- (void)testScrollToIndex0WithPadding {
  UIEdgeInsets padding = UIEdgeInsetsMake(0, 10, 0, 10);
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:padding
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:0]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, 10);
}

- (void)testScrollToIndexLast {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:9]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentOffset.x,
                 scrollView.contentSize.width - scrollView.frame.size.width);
}

- (void)testScrollToIndexLastWithPadding {
  UIEdgeInsets padding = UIEdgeInsetsMake(0, 10, 0, 10);
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:padding
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:9]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentOffset.x,
                 scrollView.contentSize.width - scrollView.frame.size.width);
}

- (void)testScrollToIndexNegative {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:-1]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, 0);
}

- (void)testScrollToIndexNegativeWithPadding {
  UIEdgeInsets padding = UIEdgeInsetsMake(0, 10, 0, 10);
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:padding
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:-1]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x, 0);
}

- (void)testScrollToIndexNormal {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:5]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x,
                 5 * 100.0f + mockContext.mockUI.padding.left);
}

- (void)testScrollToIndexNormalWithPadding {
  UIEdgeInsets padding = UIEdgeInsetsMake(0, 10, 0, 10);
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:padding
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:0],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:5]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x,
                 5 * 100.0f + mockContext.mockUI.padding.left);
}

#pragma mark scrollTo - index + offset
- (void)testScrollToOffsetPositiveIndexNormal {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  float targetOffset = 100;
  float targetIndex = 1;
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:targetOffset],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:targetIndex]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x,
                 targetIndex * 100.0f + targetOffset);
}

- (void)testScrollToOffsetNegativeIndexNormal {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  float targetOffset = -100;
  float targetIndex = 5;
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:targetOffset],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:targetIndex]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(((UIScrollView *)mockContext.mockUI.view).contentOffset.x,
                 targetIndex * 100.0f + targetOffset);
}

- (void)testScrollToOffsetNegativeIndexLast {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  float targetOffset = -100;
  float targetIndex = 9;
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:targetOffset],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:targetIndex]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  CGFloat maxScrollDistance = scrollView.contentSize.width - scrollView.frame.size.width;
  XCTAssertEqual(scrollView.contentOffset.x,
                 MIN(targetIndex * 100.0f + targetOffset, maxScrollDistance));
}

- (void)testScrollToOffsetPositiveIndexLast {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  float targetOffset = 100;
  float targetIndex = 9;
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:targetOffset],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:targetIndex]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentOffset.x,
                 scrollView.contentSize.width - scrollView.frame.size.width);
}

#pragma mark callback
- (void)testScrollToCallbackNormal {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  LynxUIMethodCallbackBlock noChildCallBack = ^(int code, id _Nullable data) {
    NSLog(@"noChildCallBack");
    XCTAssertEqual(code, kUIMethodParamInvalid);
  };
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : @(0),
                             @"smooth" : @(NO),
                             @"index" : @(0),
                           }
                           withResult:noChildCallBack
                                forUI:mockContext.mockUI];

  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:false
                                       size:CGSizeMake(100.0f, 100.0f)];

  float targetOffset = 100;
  float targetIndex = 9;
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
    XCTAssertEqual(code, kUIMethodSuccess);
  };
  [LynxUIMethodProcessor invokeMethod:@"scrollTo"
                           withParams:@{
                             @"offset" : [NSNumber numberWithInt:targetOffset],
                             @"smooth" : [NSNumber numberWithBool:0],
                             @"index" : [NSNumber numberWithInt:targetIndex]
                           }
                           withResult:callBack
                                forUI:mockContext.mockUI];
  XCTAssertEqual(scrollView.contentOffset.x,
                 scrollView.contentSize.width - scrollView.frame.size.width);
}

@end
