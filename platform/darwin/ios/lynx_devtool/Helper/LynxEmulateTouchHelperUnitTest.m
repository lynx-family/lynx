// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxEmulateTouchHelper.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

static NSString* const kScreenshotModeLynxView = @"lynxview";
static NSString* const kScreenshotModeFullScreen = @"fullscreen";

@interface LynxEmulateTouchGestureRecorder : NSObject

@property(nonatomic, assign) NSInteger tapCount;
@property(nonatomic, strong) NSMutableArray<NSNumber*>* panStates;

@end

@implementation LynxEmulateTouchGestureRecorder

- (instancetype)init {
  if (self = [super init]) {
    _panStates = [NSMutableArray array];
  }
  return self;
}

- (void)onTap:(UITapGestureRecognizer*)recognizer {
  self.tapCount++;
}

- (void)onPan:(UIPanGestureRecognizer*)recognizer {
  [self.panStates addObject:@(recognizer.state)];
}

@end

@interface LynxEmulateTouchHelperUnitTest : XCTestCase
@end

@implementation LynxEmulateTouchHelperUnitTest {
  UIWindow* _window;
  UIView* _lynxView;
  UIView* _target;
  LynxEmulateTouchGestureRecorder* _recorder;
  LynxEmulateTouchHelper* _helper;
}

- (void)setUp {
  [super setUp];
  _window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, 320, 640)];
  // The view standing in for the LynxView is offset so that coordinates in the two screenshot
  // modes differ.
  _lynxView = [[UIView alloc] initWithFrame:CGRectMake(0, 100, 320, 400)];
  _target = [[UIView alloc] initWithFrame:CGRectMake(20, 20, 100, 100)];
  _recorder = [[LynxEmulateTouchGestureRecorder alloc] init];
  [_target addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:_recorder
                                                                        action:@selector(onTap:)]];
  [_target addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:_recorder
                                                                        action:@selector(onPan:)]];
  [_lynxView addSubview:_target];
  [_window addSubview:_lynxView];
  [_window makeKeyAndVisible];
  // The helper only relies on the view hierarchy of the LynxView.
  _helper = [[LynxEmulateTouchHelper alloc] initWithLynxView:(LynxView*)_lynxView];
}

- (void)tearDown {
  _window.hidden = YES;
  _helper = nil;
  _recorder = nil;
  _target = nil;
  _lynxView = nil;
  _window = nil;
  [super tearDown];
}

// Synthesized touches only reach gesture recognizers when UIKit delivers them through a running
// UIApplication, which the standalone test runner does not provide. The touch state kept by the
// helper is verified everywhere and recognizer callbacks only when an application exists.
- (BOOL)deliversTouches {
  return [UIApplication sharedApplication] != nil;
}

- (void)emulate:(NSString*)type x:(int)x y:(int)y mode:(NSString*)mode {
  [self emulate:type x:x y:y deltaX:0 deltaY:0 mode:mode];
}

- (void)emulate:(NSString*)type
              x:(int)x
              y:(int)y
         deltaX:(CGFloat)deltaX
         deltaY:(CGFloat)deltaY
           mode:(NSString*)mode {
  [_helper emulateTouch:type
            coordinateX:x
            coordinateY:y
                 button:@"left"
                 deltaX:deltaX
                 deltaY:deltaY
              modifiers:0
             clickCount:1
         screenshotMode:mode];
}

- (void)runLoopForSeconds:(NSTimeInterval)seconds {
  [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:seconds]];
}

- (BOOL)waitUntil:(BOOL (^)(void))condition timeout:(NSTimeInterval)timeout {
  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:timeout];
  while (!condition() && [deadline timeIntervalSinceNow] > 0) {
    [self runLoopForSeconds:0.02];
  }
  return condition();
}

- (BOOL)waitForTapCount:(NSInteger)count timeout:(NSTimeInterval)timeout {
  return [self
      waitUntil:^BOOL {
        return self->_recorder.tapCount >= count;
      }
        timeout:timeout];
}

- (BOOL)waitForPanState:(UIGestureRecognizerState)state timeout:(NSTimeInterval)timeout {
  return [self
      waitUntil:^BOOL {
        return [self->_recorder.panStates containsObject:@(state)];
      }
        timeout:timeout];
}

- (void)assertLastLocation:(CGPoint)expected {
  XCTAssertEqualWithAccuracy(_helper.last.x, expected.x, 0.001);
  XCTAssertEqualWithAccuracy(_helper.last.y, expected.y, 0.001);
}

- (void)testPressAndReleaseTriggersTapInLynxViewCoordinates {
  [self emulate:@"mousePressed" x:50 y:50 mode:kScreenshotModeLynxView];
  XCTAssertNotNil(_helper.touch);
  XCTAssertEqual(_helper.touch.phase, UITouchPhaseBegan);
  XCTAssertEqual(_helper.touch.view, _target);
  [self assertLastLocation:CGPointMake(50, 150)];

  [self emulate:@"mouseReleased" x:50 y:50 mode:kScreenshotModeLynxView];
  XCTAssertNil(_helper.touch);
  if ([self deliversTouches]) {
    XCTAssertTrue([self waitForTapCount:1 timeout:1]);
  }
}

- (void)testFullScreenModeUsesWindowCoordinates {
  // (50, 50) in window coordinates lies above the LynxView.
  [self emulate:@"mousePressed" x:50 y:50 mode:kScreenshotModeFullScreen];
  XCTAssertNotNil(_helper.touch);
  XCTAssertNotEqual(_helper.touch.view, _target);
  [self assertLastLocation:CGPointMake(50, 50)];
  [self emulate:@"mouseReleased" x:50 y:50 mode:kScreenshotModeFullScreen];
  [self runLoopForSeconds:0.1];
  XCTAssertEqual(_recorder.tapCount, 0);

  [self emulate:@"mousePressed" x:50 y:150 mode:kScreenshotModeFullScreen];
  XCTAssertEqual(_helper.touch.view, _target);
  [self assertLastLocation:CGPointMake(50, 150)];
  [self emulate:@"mouseReleased" x:50 y:150 mode:kScreenshotModeFullScreen];
  if ([self deliversTouches]) {
    XCTAssertTrue([self waitForTapCount:1 timeout:1]);
  }
}

- (void)testMoveAndReleaseWithoutPressAreIgnored {
  [self emulate:@"mouseMoved" x:50 y:50 mode:kScreenshotModeLynxView];
  XCTAssertNil(_helper.touch);
  [self emulate:@"mouseReleased" x:50 y:50 mode:kScreenshotModeLynxView];
  XCTAssertNil(_helper.touch);
  [self runLoopForSeconds:0.1];

  XCTAssertEqual(_recorder.tapCount, 0);
  XCTAssertEqual(_recorder.panStates.count, 0u);
}

- (void)testDragTriggersPanInsteadOfTap {
  [self emulate:@"mousePressed" x:50 y:50 mode:kScreenshotModeLynxView];
  UITouch* touch = _helper.touch;
  [self emulate:@"mouseMoved" x:50 y:80 mode:kScreenshotModeLynxView];
  XCTAssertEqual(_helper.touch, touch);
  XCTAssertEqual(touch.phase, UITouchPhaseMoved);
  [self assertLastLocation:CGPointMake(50, 180)];
  [self emulate:@"mouseMoved" x:50 y:110 mode:kScreenshotModeLynxView];
  [self assertLastLocation:CGPointMake(50, 210)];
  [self emulate:@"mouseReleased" x:50 y:110 mode:kScreenshotModeLynxView];
  XCTAssertNil(_helper.touch);

  if ([self deliversTouches]) {
    XCTAssertTrue([self waitForPanState:UIGestureRecognizerStateEnded timeout:1]);
    XCTAssertTrue([_recorder.panStates containsObject:@(UIGestureRecognizerStateBegan)]);
  }
  XCTAssertEqual(_recorder.tapCount, 0);
}

- (void)testMouseWheelEmulatesDragAndReleasesWhenIdle {
  [self emulate:@"mouseWheel" x:50 y:50 deltaX:0 deltaY:-100 mode:kScreenshotModeLynxView];
  XCTAssertNotNil(_helper.touch);
  XCTAssertTrue(_helper.mouseWheelFlag);
  [self assertLastLocation:CGPointMake(50, 150 - 100.0 / _helper.deltaScale)];
  [self emulate:@"mouseWheel" x:50 y:50 deltaX:0 deltaY:-100 mode:kScreenshotModeLynxView];
  [self assertLastLocation:CGPointMake(50, 150 - 200.0 / _helper.deltaScale)];

  // Mouse moves do not disturb a drag driven by the wheel.
  [self emulate:@"mouseMoved" x:200 y:200 mode:kScreenshotModeLynxView];
  [self assertLastLocation:CGPointMake(50, 150 - 200.0 / _helper.deltaScale)];

  // The drag is released once no wheel event arrives for a while.
  XCTAssertTrue([self
      waitUntil:^BOOL {
        return self->_helper.touch == nil;
      }
        timeout:2]);
  XCTAssertFalse(_helper.mouseWheelFlag);

  if ([self deliversTouches]) {
    XCTAssertTrue([self waitForPanState:UIGestureRecognizerStateBegan timeout:1]);
    XCTAssertTrue([self waitForPanState:UIGestureRecognizerStateEnded timeout:1]);
  }
  XCTAssertEqual(_recorder.tapCount, 0);
}

- (void)testMouseWheelIsIgnoredWhilePressed {
  [self emulate:@"mousePressed" x:50 y:50 mode:kScreenshotModeLynxView];
  [self emulate:@"mouseWheel" x:50 y:50 deltaX:0 deltaY:-100 mode:kScreenshotModeLynxView];
  XCTAssertFalse(_helper.mouseWheelFlag);
  [self assertLastLocation:CGPointMake(50, 150)];
  [self emulate:@"mouseReleased" x:50 y:50 mode:kScreenshotModeLynxView];
  XCTAssertNil(_helper.touch);
}

- (void)testNewPressReplacesActiveTouch {
  [self emulate:@"mousePressed" x:50 y:50 mode:kScreenshotModeLynxView];
  UITouch* first = _helper.touch;
  [self emulate:@"mousePressed" x:60 y:60 mode:kScreenshotModeLynxView];
  XCTAssertNotNil(_helper.touch);
  XCTAssertNotEqual(_helper.touch, first);
  XCTAssertEqual(first.phase, UITouchPhaseCancelled);
  [self assertLastLocation:CGPointMake(60, 160)];
}

- (void)testAttachLynxViewCancelsPendingTouch {
  [self emulate:@"mousePressed" x:50 y:50 mode:kScreenshotModeLynxView];
  UITouch* touch = _helper.touch;
  [_helper attachLynxView:(LynxView*)_lynxView];
  XCTAssertEqual(touch.phase, UITouchPhaseCancelled);
  XCTAssertNil(_helper.touch);
  XCTAssertFalse(_helper.mouseWheelFlag);
  [self emulate:@"mouseReleased" x:50 y:50 mode:kScreenshotModeLynxView];
  XCTAssertNil(_helper.touch);
  [self runLoopForSeconds:0.1];

  XCTAssertEqual(_recorder.tapCount, 0);
}

@end
