// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxPointerEventDispatcher.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "devtool/lynx_devtool/input/input_event_target.h"

@interface LynxPointerEventDispatcher (PointerEventInjectionTesting)

- (BOOL)lynx_isPointerEventInjectionAvailable;
- (BOOL)lynx_dispatchPointerEvent:(LynxDevToolPointerEventType)type
                         inWindow:(UIWindow *)window
                        hitTarget:(nullable UIView *)target
                       coordinate:(CGPoint)point;
- (void)lynx_cancelCurrentPointerSequence;

@end

@interface DevToolInsertTextField : UITextField
@end

@implementation DevToolInsertTextField

- (BOOL)isFirstResponder {
  return YES;
}

@end

// Stubs the typed injection entry point so the native adapter can be exercised
// without touching real UIKit.
@interface DevToolPointerInjectionHelper : LynxPointerEventDispatcher

@property(nonatomic, assign) BOOL injectionAvailable;
@property(nonatomic, assign) BOOL injectionResult;
@property(nonatomic, assign) BOOL injectedOnMainThread;
@property(nonatomic, assign) NSInteger injectionCount;
@property(nonatomic, assign) NSInteger cancellationCount;

@end

@implementation DevToolPointerInjectionHelper

- (BOOL)isPointerEventInjectionAvailable {
  return self.injectionAvailable;
}

- (BOOL)injectPointerEvent:(LynxDevToolPointerEventType)type
               coordinateX:(CGFloat)x
               coordinateY:(CGFloat)y
                    deltaX:(CGFloat)deltaX
                    deltaY:(CGFloat)deltaY
                 pointerId:(int32_t)pointerId
                 modifiers:(int32_t)modifiers
               timestampUs:(int64_t)timestampUs {
  self.injectedOnMainThread = [NSThread isMainThread];
  self.injectionCount += 1;
  return self.injectionResult;
}

- (void)cancelCurrentPointerSequence {
  self.cancellationCount += 1;
}

@end

// Stubs only the private UIKit dispatch so the dispatcher's own sequence state
// machine (window resolution, boundary checks, cancellation) can be tested.
@interface DevToolWindowInputHelper : LynxPointerEventDispatcher

@property(nonatomic, assign) BOOL dispatchResult;
@property(nonatomic, assign) NSInteger dispatchCount;
@property(nonatomic, assign) NSInteger cancellationCount;
@property(nonatomic, weak, nullable) UIWindow *lastWindow;
@property(nonatomic, weak, nullable) UIView *lastTarget;

@end

@implementation DevToolWindowInputHelper

- (instancetype)initWithLynxView:(nullable LynxView *)view {
  self = [super initWithLynxView:view];
  if (self) {
    _dispatchResult = YES;
  }
  return self;
}

- (BOOL)lynx_isPointerEventInjectionAvailable {
  return YES;
}

- (BOOL)lynx_dispatchPointerEvent:(LynxDevToolPointerEventType)type
                         inWindow:(UIWindow *)window
                        hitTarget:(nullable UIView *)target
                       coordinate:(CGPoint)point {
  self.dispatchCount += 1;
  self.lastWindow = window;
  self.lastTarget = target;
  return self.dispatchResult;
}

- (void)lynx_cancelCurrentPointerSequence {
  self.cancellationCount += 1;
}

@end

namespace {

lynx::devtool::input::PointerEvent MakePointerEvent(
    lynx::devtool::input::PointerSourceType source_type,
    lynx::devtool::input::PointerEventType event_type, float x, float y) {
  lynx::devtool::input::PointerEvent event;
  event.source_type = source_type;
  event.type = event_type;
  event.action_pointer_id = 1;
  lynx::devtool::input::Pointer pointer;
  pointer.id = 1;
  pointer.x = x;
  pointer.y = y;
  event.pointers.push_back(pointer);
  return event;
}

UIWindow *MakeWindow(CGSize size) {
  UIWindow *window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, size.width, size.height)];
  window.hidden = NO;
  UIView *root = [[UIView alloc] initWithFrame:window.bounds];
  [window addSubview:root];
  return window;
}

UIView *WindowRootView(UIWindow *window) { return window.subviews.firstObject; }

}  // namespace

@interface DevToolPlatformDarwinDelegateUnitTest : XCTestCase
@end

@implementation DevToolPlatformDarwinDelegateUnitTest

- (void)testInsertTextUsesFirstResponderTextInput {
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectZero];
  UIView *container = [[UIView alloc] initWithFrame:CGRectZero];
  DevToolInsertTextField *textField = [[DevToolInsertTextField alloc] initWithFrame:CGRectZero];
  textField.text = @"ac";
  textField.selectedTextRange = [textField
      textRangeFromPosition:[textField positionFromPosition:textField.beginningOfDocument offset:1]
                 toPosition:[textField positionFromPosition:textField.beginningOfDocument
                                                     offset:1]];
  [container addSubview:textField];
  [lynxView addSubview:container];

  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:(LynxView *)lynxView];

  [platform insertText:@"b"];

  XCTAssertEqualObjects(textField.text, @"abc");
}

- (void)testInputEventTargetCapabilitiesFollowInternalUIKitAvailability {
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];
  DevToolPointerInjectionHelper *helper =
      [[DevToolPointerInjectionHelper alloc] initWithLynxView:nil];
  helper.injectionAvailable = YES;
  [platform setValue:helper forKey:@"pointerEventDispatcher"];

  std::shared_ptr<lynx::devtool::input::InputEventTarget> target =
      [platform getNativePtr]->GetInputEventTarget();
  XCTAssertNotEqual(target, nullptr);
  auto capabilities = target->GetPointerCapabilities();
  XCTAssertEqual(capabilities.default_source_type, lynx::devtool::input::PointerSourceType::kTouch);
  XCTAssertTrue(capabilities.supports_touch);
  XCTAssertFalse(capabilities.supports_mouse);

  helper.injectionAvailable = NO;
  capabilities = target->GetPointerCapabilities();
  XCTAssertEqual(capabilities.default_source_type,
                 lynx::devtool::input::PointerSourceType::kDefault);
  XCTAssertFalse(capabilities.supports_touch);
  XCTAssertFalse(capabilities.supports_mouse);
}

- (void)testInputEventTargetRejectsInvalidSourceAndMultiplePointers {
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];
  DevToolPointerInjectionHelper *helper =
      [[DevToolPointerInjectionHelper alloc] initWithLynxView:nil];
  helper.injectionAvailable = YES;
  helper.injectionResult = YES;
  [platform setValue:helper forKey:@"pointerEventDispatcher"];
  auto target = [platform getNativePtr]->GetInputEventTarget();

  auto event = MakePointerEvent(lynx::devtool::input::PointerSourceType::kMouse,
                                lynx::devtool::input::PointerEventType::kDown, 10.f, 20.f);
  XCTAssertFalse(target->InjectPointerEvent(event));

  event.source_type = lynx::devtool::input::PointerSourceType::kTouch;
  lynx::devtool::input::Pointer secondPointer;
  secondPointer.id = 2;
  secondPointer.x = 30.f;
  secondPointer.y = 40.f;
  event.pointers.push_back(secondPointer);
  XCTAssertFalse(target->InjectPointerEvent(event));
  XCTAssertEqual(helper.injectionCount, 0);
}

- (void)testPointerHelperRejectsNilWindowAndWindowBoundary {
  UIView *detachedView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 20, 20)];
  DevToolWindowInputHelper *helper =
      [[DevToolWindowInputHelper alloc] initWithLynxView:(LynxView *)detachedView];
  XCTAssertFalse([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                                coordinateX:10
                                coordinateY:10
                                     deltaX:0
                                     deltaY:0
                                  pointerId:1
                                  modifiers:0
                                timestampUs:0]);

  UIWindow *window = MakeWindow(CGSizeMake(200, 100));
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 20, 20)];
  [WindowRootView(window) addSubview:lynxView];
  [helper attachLynxView:(LynxView *)lynxView];

  XCTAssertFalse([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                                coordinateX:-1
                                coordinateY:10
                                     deltaX:0
                                     deltaY:0
                                  pointerId:1
                                  modifiers:0
                                timestampUs:0]);
  XCTAssertFalse([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                                coordinateX:200
                                coordinateY:10
                                     deltaX:0
                                     deltaY:0
                                  pointerId:1
                                  modifiers:0
                                timestampUs:0]);
  XCTAssertFalse([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                                coordinateX:10
                                coordinateY:100
                                     deltaX:0
                                     deltaY:0
                                  pointerId:1
                                  modifiers:0
                                timestampUs:0]);
  XCTAssertEqual(helper.dispatchCount, 0);
}

- (void)testPointerHelperUsesWindowCoordinatesForHitTestingOutsideLynxView {
  UIWindow *window = MakeWindow(CGSizeMake(200, 200));
  UIView *root = WindowRootView(window);
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectMake(40, 50, 40, 40)];
  UIView *hostView = [[UIView alloc] initWithFrame:CGRectMake(100, 100, 40, 40)];
  [root addSubview:lynxView];
  [root addSubview:hostView];
  DevToolWindowInputHelper *helper =
      [[DevToolWindowInputHelper alloc] initWithLynxView:(LynxView *)lynxView];
  CGPoint windowPoint = CGPointMake(110, 110);
  CGPoint pointIfTreatedAsLynxViewLocal = [lynxView convertPoint:windowPoint toView:window];
  XCTAssertEqual([window hitTest:windowPoint withEvent:nil], hostView);
  XCTAssertNotEqual([window hitTest:pointIfTreatedAsLynxViewLocal withEvent:nil], hostView);

  XCTAssertTrue([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                               coordinateX:windowPoint.x
                               coordinateY:windowPoint.y
                                    deltaX:0
                                    deltaY:0
                                 pointerId:1
                                 modifiers:0
                               timestampUs:0]);
  XCTAssertEqual(helper.lastWindow, window);
  XCTAssertEqual(helper.lastTarget, hostView);
  XCTAssertTrue([helper injectPointerEvent:LynxDevToolPointerEventTypeUp
                               coordinateX:windowPoint.x
                               coordinateY:windowPoint.y
                                    deltaX:0
                                    deltaY:0
                                 pointerId:1
                                 modifiers:0
                               timestampUs:0]);
}

- (void)testPointerHelperCancelsOnWindowSwitchAndAttach {
  UIWindow *firstWindow = MakeWindow(CGSizeMake(100, 100));
  UIWindow *secondWindow = MakeWindow(CGSizeMake(100, 100));
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 50, 50)];
  [WindowRootView(firstWindow) addSubview:lynxView];
  DevToolWindowInputHelper *helper =
      [[DevToolWindowInputHelper alloc] initWithLynxView:(LynxView *)lynxView];

  XCTAssertTrue([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                               coordinateX:10
                               coordinateY:10
                                    deltaX:0
                                    deltaY:0
                                 pointerId:1
                                 modifiers:0
                               timestampUs:0]);
  [WindowRootView(secondWindow) addSubview:lynxView];
  XCTAssertFalse([helper injectPointerEvent:LynxDevToolPointerEventTypeMove
                                coordinateX:12
                                coordinateY:12
                                     deltaX:0
                                     deltaY:0
                                  pointerId:1
                                  modifiers:0
                                timestampUs:0]);
  XCTAssertEqual(helper.cancellationCount, 1);

  XCTAssertTrue([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                               coordinateX:10
                               coordinateY:10
                                    deltaX:0
                                    deltaY:0
                                 pointerId:2
                                 modifiers:0
                               timestampUs:0]);
  UIView *replacement = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 50, 50)];
  [WindowRootView(secondWindow) addSubview:replacement];
  [helper attachLynxView:(LynxView *)replacement];
  XCTAssertEqual(helper.cancellationCount, 2);
}

- (void)testPointerHelperCancelsWhenReleaseDispatchFails {
  UIWindow *window = MakeWindow(CGSizeMake(100, 100));
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 50, 50)];
  [WindowRootView(window) addSubview:lynxView];
  DevToolWindowInputHelper *helper =
      [[DevToolWindowInputHelper alloc] initWithLynxView:(LynxView *)lynxView];

  XCTAssertTrue([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                               coordinateX:10
                               coordinateY:10
                                    deltaX:0
                                    deltaY:0
                                 pointerId:1
                                 modifiers:0
                               timestampUs:0]);
  helper.dispatchResult = NO;
  XCTAssertFalse([helper injectPointerEvent:LynxDevToolPointerEventTypeUp
                                coordinateX:10
                                coordinateY:10
                                     deltaX:0
                                     deltaY:0
                                  pointerId:1
                                  modifiers:0
                                timestampUs:0]);
  XCTAssertEqual(helper.cancellationCount, 1);

  helper.dispatchResult = YES;
  XCTAssertTrue([helper injectPointerEvent:LynxDevToolPointerEventTypeDown
                               coordinateX:10
                               coordinateY:10
                                    deltaX:0
                                    deltaY:0
                                 pointerId:2
                                 modifiers:0
                               timestampUs:0]);
}

- (void)testDelegateDeallocCancelsActivePointerSequence {
  DevToolPointerInjectionHelper *helper =
      [[DevToolPointerInjectionHelper alloc] initWithLynxView:nil];
  @autoreleasepool {
    DevToolPlatformDarwinDelegate *platform =
        [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];
    [platform setValue:helper forKey:@"pointerEventDispatcher"];
    platform = nil;
  }
  XCTAssertEqual(helper.cancellationCount, 1);
}

- (void)testInputEventTargetRoutesInjectionToMainThread {
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];
  DevToolPointerInjectionHelper *helper =
      [[DevToolPointerInjectionHelper alloc] initWithLynxView:nil];
  helper.injectionAvailable = YES;
  helper.injectionResult = YES;
  [platform setValue:helper forKey:@"pointerEventDispatcher"];
  auto target = [platform getNativePtr]->GetInputEventTarget();
  auto event = MakePointerEvent(lynx::devtool::input::PointerSourceType::kTouch,
                                lynx::devtool::input::PointerEventType::kDown, 10.f, 20.f);
  XCTestExpectation *expectation = [self expectationWithDescription:@"main thread injection"];

  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    XCTAssertTrue(target->InjectPointerEvent(event));
    XCTAssertTrue(helper.injectedOnMainThread);
    [expectation fulfill];
  });

  [self waitForExpectations:@[ expectation ] timeout:2];
}

@end
