// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventHandlerUnitTest.h"
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTouchHandler.h>
#import <Lynx/LynxTransformRaw.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxWeakProxy.h>

@interface LynxUI ()
@property(nonatomic, weak) LynxUIContext* context;
- (Class)iosPanInterceptViewClass;
- (Class)iosPanInterceptGestureClass;
- (NSInteger)iosPanInterceptViewTag;
- (BOOL)hasIosPanInterceptViewTag;
@end

@interface LynxUIContext ()
@property(nonatomic, weak) LynxEventHandler* eventHandler;
@end

@interface LynxPanInterceptUnitTestGesture : UIGestureRecognizer
@property(nonatomic, assign) UIGestureRecognizerState testState;
@end

@implementation LynxPanInterceptUnitTestGesture
- (UIGestureRecognizerState)state {
  return self.testState;
}
- (void)setState:(UIGestureRecognizerState)state {
  self.testState = state;
}
@end

@interface LynxPanInterceptUnitTestView : UIView
@end

@implementation LynxPanInterceptUnitTestView
@end

static const NSInteger kLynxPanInterceptUnitTestViewTag = 1001;

@implementation LynxEventHanderUnitTest {
  LynxEventHandler* _handler;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  _handler = OCMPartialMock([[LynxEventHandler alloc] initWithRootView:rootUI.view
                                                            withRootUI:rootUI
                                                               andFlag:NO]);
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  _handler = NULL;
}

- (void)testOnGestureRecognizedByEventTargetAsync {
  LynxUIView* mockUI1 = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  LynxUIView* mockUI2 = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  [mockUI1 insertChild:mockUI2 atIndex:0];

  id mockCtx = OCMPartialMock([[LynxUIContext alloc] init]);
  ((LynxUIContext*)mockCtx).eventHandler = _handler;
  OCMStub([mockCtx isTouchMoving]).andReturn(YES);
  NSArray* transformRaw = @[ @[
    @(LynxTransformTypeTranslateX), @10, @(LynxPlatformLengthUnitNumber), @0,
    @(LynxPlatformLengthUnitNumber), @0, @(LynxPlatformLengthUnitNumber)
  ] ];

  dispatch_queue_t concurrentQueue1 = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_queue_t concurrentQueue2 = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  NSMutableArray* uiArray = [NSMutableArray new];
  for (size_t i = 0; i < 2000; ++i) {
    LynxUI* ui = [[LynxUI alloc] init];
    ui.context = mockCtx;
    ui.sign = i;
    [uiArray addObject:ui];
  }
  dispatch_async(concurrentQueue1, ^{
    for (size_t i = 0; i < 1000; ++i) {
      [LynxPropsProcessor updateProp:transformRaw withKey:@"transform" forUI:uiArray[i]];
    }
  });
  dispatch_async(concurrentQueue2, ^{
    for (size_t i = 1000; i < 2000; ++i) {
      [LynxPropsProcessor updateProp:transformRaw withKey:@"transform" forUI:uiArray[i]];
    }
  });

  NSDate* timeoutDate = [NSDate dateWithTimeIntervalSinceNow:6];
  while ([timeoutDate timeIntervalSinceNow] > 0) {
    [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                          beforeDate:[NSDate dateWithTimeIntervalSinceNow:1]];
  }
  [uiArray enumerateObjectsUsingBlock:^(id _Nonnull ui, NSUInteger idx, BOOL* _Nonnull stop) {
    [(LynxUI*)ui performSelector:@selector(onNodeReadyForUIOwner)];
  }];
  XCTAssertEqual([(NSMutableSet*)[_handler valueForKey:@"_setOfPropsChanged"] count], 2000);
}

- (void)testIsFragmentLayerRenderOn {
  UIView* mockView = OCMClassMock([UIView class]);
  LynxRootUI* mockRootUI = OCMClassMock([LynxRootUI class]);

  // Test 1: Flag is YES
  // When flag is YES, tap and long press recognizers should NOT be added.
  // Only LynxTouchHandler should be added.
  OCMReject([mockView addGestureRecognizer:[OCMArg checkWithBlock:^BOOL(id obj) {
                        return [obj isKindOfClass:[UITapGestureRecognizer class]] ||
                               [obj isKindOfClass:[UILongPressGestureRecognizer class]];
                      }]]);

  LynxEventHandler* handlerOn = [[LynxEventHandler alloc] initWithRootView:mockView
                                                                withRootUI:mockRootUI
                                                                   andFlag:YES];
  OCMVerify([mockView addGestureRecognizer:[OCMArg isKindOfClass:[LynxTouchHandler class]]]);
  XCTAssertTrue([[handlerOn valueForKey:@"_isFragmentLayerRendererOn"] boolValue]);

  // Test 2: Flag is NO
  // When flag is NO, tap and long press recognizers SHOULD be added.
  id mockView2 = OCMClassMock([UIView class]);
  LynxEventHandler* handlerOff = [[LynxEventHandler alloc] initWithRootView:mockView2
                                                                 withRootUI:mockRootUI
                                                                    andFlag:NO];
  OCMVerify([mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[UITapGestureRecognizer class]]]);
  OCMVerify(
      [mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[UILongPressGestureRecognizer class]]]);
  XCTAssertFalse([[handlerOff valueForKey:@"_isFragmentLayerRendererOn"] boolValue]);
}

- (void)testSetupPlatformGestureWithFragmentLayerRenderOn {
  UIView* mockView = OCMClassMock([UIView class]);
  LynxRootUI* mockRootUI = OCMClassMock([LynxRootUI class]);

  LynxEventHandler* handlerOn = [[LynxEventHandler alloc] initWithRootView:mockView
                                                                withRootUI:mockRootUI
                                                                   andFlag:YES];

  // When flag is YES, setUpPlatformGesture should do nothing
  [handlerOn setEnablePlatformGesture:YES];

  // Verify that LynxCustomGestureRecognizer was NOT added
  OCMVerify(
      never(),
      [mockView addGestureRecognizer:[OCMArg isKindOfClass:NSClassFromString(
                                                               @"LynxCustomGestureRecognizer")]]);
}

- (void)testAttachContainerViewWithFragmentLayerRenderOn {
  UIView* mockView1 = OCMClassMock([UIView class]);
  LynxRootUI* mockRootUI = OCMClassMock([LynxRootUI class]);

  LynxEventHandler* handlerOn = [[LynxEventHandler alloc] initWithRootView:mockView1
                                                                withRootUI:mockRootUI
                                                                   andFlag:YES];

  UIView* mockView2 = OCMClassMock([UIView class]);
  // When flag is YES, attachContainerView should NOT add tap/longpress recognizers
  OCMReject([mockView2 addGestureRecognizer:[OCMArg checkWithBlock:^BOOL(id obj) {
                         return [obj isKindOfClass:[UITapGestureRecognizer class]] ||
                                [obj isKindOfClass:[UILongPressGestureRecognizer class]];
                       }]]);

  [handlerOn attachContainerView:mockView2];
  OCMVerify([mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[LynxTouchHandler class]]]);
}

- (LynxUIView*)configuredPanInterceptTargetUIWithViewTag:(BOOL)hasViewTag {
  LynxUIView* targetUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [LynxPropsProcessor updateProp:@(kLynxPanInterceptDirectionHorizontal)
                         withKey:@"pan-intercept-direction"
                           forUI:targetUI];
  [LynxPropsProcessor updateProp:@(kLynxPanInterceptScopeAll)
                         withKey:@"pan-intercept-scope"
                           forUI:targetUI];
  [LynxPropsProcessor updateProp:NSStringFromClass([LynxPanInterceptUnitTestGesture class])
                         withKey:@"ios-pan-intercept-gesture-class"
                           forUI:targetUI];
  [LynxPropsProcessor updateProp:NSStringFromClass([LynxPanInterceptUnitTestView class])
                         withKey:@"ios-pan-intercept-view-class"
                           forUI:targetUI];
  if (hasViewTag) {
    [LynxPropsProcessor updateProp:@(kLynxPanInterceptUnitTestViewTag)
                           withKey:@"ios-pan-intercept-view-tag"
                             forUI:targetUI];
  }
  XCTAssertEqual(targetUI.iosPanInterceptGestureClass, [LynxPanInterceptUnitTestGesture class]);
  XCTAssertEqual(targetUI.iosPanInterceptViewClass, [LynxPanInterceptUnitTestView class]);
  XCTAssertEqual(targetUI.hasIosPanInterceptViewTag, hasViewTag);
  if (hasViewTag) {
    XCTAssertEqual(targetUI.iosPanInterceptViewTag, kLynxPanInterceptUnitTestViewTag);
  }
  [_handler setValue:targetUI forKey:@"_firstPanInterceptDirectionTarget"];
  return targetUI;
}

- (LynxUIView*)configuredPanInterceptTargetUI {
  return [self configuredPanInterceptTargetUIWithViewTag:YES];
}

- (void)testInterceptPanGestureWithConfiguredGestureViewClassAndTag {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUI];
  XCTAssertNotNil(targetUI);

  LynxPanInterceptUnitTestView* otherView = [[LynxPanInterceptUnitTestView alloc] init];
  otherView.tag = kLynxPanInterceptUnitTestViewTag;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStateFailed);
}

- (void)testConfiguredPanInterceptRequiresViewClassMatch {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUI];
  XCTAssertNotNil(targetUI);

  UIView* otherView = [[UIView alloc] init];
  otherView.tag = kLynxPanInterceptUnitTestViewTag;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStatePossible);
}

- (void)testConfiguredPanInterceptRequiresViewTagMatch {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUI];
  XCTAssertNotNil(targetUI);

  LynxPanInterceptUnitTestView* otherView = [[LynxPanInterceptUnitTestView alloc] init];
  otherView.tag = kLynxPanInterceptUnitTestViewTag + 1;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStatePossible);
}

- (void)testConfiguredPanInterceptRequiresViewTagProperty {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUIWithViewTag:NO];
  XCTAssertNotNil(targetUI);

  LynxPanInterceptUnitTestView* otherView = [[LynxPanInterceptUnitTestView alloc] init];
  otherView.tag = 0;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStatePossible);
}

@end
