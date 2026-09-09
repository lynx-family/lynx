// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxEmulateTouchHelper.h>

#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIKitAPIAdapter.h>

#include <dlfcn.h>
#include <mach/mach_time.h>

// Event types defined by the CDP method `Input.emulateTouchFromMouseEvent`.
static NSString* const kMouseEventPressed = @"mousePressed";
static NSString* const kMouseEventMoved = @"mouseMoved";
static NSString* const kMouseEventReleased = @"mouseReleased";
static NSString* const kMouseEventWheel = @"mouseWheel";

static NSString* const kScreenshotModeLynxView = @"lynxview";

// A mouse wheel sequence is emulated as a drag which is released when no wheel event arrives
// within this interval, consistent with the Android implementation.
static int64_t const kMouseWheelReleaseDelayMs = 100;

#pragma mark - IOHIDEvent

// UIKit does not deliver touches without an IOHIDEvent to gesture recognizers, so a digitizer
// event is attached to every emulated touch. The IOKit symbols are resolved at runtime and touch
// emulation is disabled when they are unavailable.
typedef struct __IOHIDEvent* IOHIDEventRef;
// IOHIDFloat follows the same LP64 rule as CGFloat: double on 64-bit and float on 32-bit.
typedef CGFloat IOHIDFloat;

static uint32_t const kIOHIDEventTypeDigitizer = 11;
static uint32_t const kIOHIDDigitizerTransducerTypeHand = 3;
static uint32_t const kIOHIDDigitizerEventRange = 1 << 0;
static uint32_t const kIOHIDDigitizerEventTouch = 1 << 1;
static uint32_t const kIOHIDDigitizerEventPosition = 1 << 2;
// IOHIDEventFieldBase(kIOHIDEventTypeDigitizer) plus the offset of
// kIOHIDEventFieldDigitizerIsDisplayIntegrated in IOHIDEventTypes.h.
static uint32_t const kIOHIDEventFieldDigitizerIsDisplayIntegrated =
    (kIOHIDEventTypeDigitizer << 16) | 25;

typedef IOHIDEventRef (*LynxIOHIDEventCreateDigitizerEventFunc)(
    CFAllocatorRef allocator, AbsoluteTime timeStamp, uint32_t type, uint32_t index,
    uint32_t identity, uint32_t eventMask, uint32_t buttonMask, IOHIDFloat x, IOHIDFloat y,
    IOHIDFloat z, IOHIDFloat tipPressure, IOHIDFloat barrelPressure, Boolean range, Boolean touch,
    uint32_t options);
typedef IOHIDEventRef (*LynxIOHIDEventCreateDigitizerFingerEventFunc)(
    CFAllocatorRef allocator, AbsoluteTime timeStamp, uint32_t index, uint32_t identity,
    uint32_t eventMask, IOHIDFloat x, IOHIDFloat y, IOHIDFloat z, IOHIDFloat tipPressure,
    IOHIDFloat twist, Boolean range, Boolean touch, uint32_t options);
typedef void (*LynxIOHIDEventAppendEventFunc)(IOHIDEventRef event, IOHIDEventRef childEvent,
                                              uint32_t options);
typedef void (*LynxIOHIDEventSetIntegerValueFunc)(IOHIDEventRef event, uint32_t field,
                                                  CFIndex value);

typedef struct {
  LynxIOHIDEventCreateDigitizerEventFunc createDigitizerEvent;
  LynxIOHIDEventCreateDigitizerFingerEventFunc createDigitizerFingerEvent;
  LynxIOHIDEventAppendEventFunc appendEvent;
  LynxIOHIDEventSetIntegerValueFunc setIntegerValue;
} LynxIOHIDEventApi;

static const LynxIOHIDEventApi* LynxGetIOHIDEventApi(void) {
  static LynxIOHIDEventApi api;
  static BOOL available = NO;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    void* handle = dlopen("/System/Library/Frameworks/IOKit.framework/IOKit", RTLD_LAZY);
    if (handle == NULL) {
      handle = RTLD_DEFAULT;
    }
    api.createDigitizerEvent =
        (LynxIOHIDEventCreateDigitizerEventFunc)dlsym(handle, "IOHIDEventCreateDigitizerEvent");
    api.createDigitizerFingerEvent = (LynxIOHIDEventCreateDigitizerFingerEventFunc)dlsym(
        handle, "IOHIDEventCreateDigitizerFingerEvent");
    api.appendEvent = (LynxIOHIDEventAppendEventFunc)dlsym(handle, "IOHIDEventAppendEvent");
    api.setIntegerValue =
        (LynxIOHIDEventSetIntegerValueFunc)dlsym(handle, "IOHIDEventSetIntegerValue");
    available = api.createDigitizerEvent != NULL && api.createDigitizerFingerEvent != NULL &&
                api.appendEvent != NULL && api.setIntegerValue != NULL;
    if (!available) {
      LLogError(@"LynxEmulateTouchHelper: IOHIDEvent symbols are unavailable");
    }
  });
  return available ? &api : NULL;
}

// Returns a retained digitizer event describing the current state of the touch.
static IOHIDEventRef LynxCreateIOHIDEventForTouch(UITouch* touch) {
  const LynxIOHIDEventApi* api = LynxGetIOHIDEventApi();
  if (api == NULL) {
    return NULL;
  }
  uint64_t machTime = mach_absolute_time();
  AbsoluteTime timestamp;
  timestamp.hi = (UInt32)(machTime >> 32);
  timestamp.lo = (UInt32)(machTime & 0xFFFFFFFF);

  IOHIDEventRef handEvent =
      api->createDigitizerEvent(kCFAllocatorDefault, timestamp, kIOHIDDigitizerTransducerTypeHand,
                                0, 0, kIOHIDDigitizerEventTouch, 0, 0, 0, 0, 0, 0, false, true, 0);
  if (handEvent == NULL) {
    return NULL;
  }
  api->setIntegerValue(handEvent, kIOHIDEventFieldDigitizerIsDisplayIntegrated, 1);

  Boolean touching = touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled;
  uint32_t eventMask = touch.phase == UITouchPhaseMoved
                           ? kIOHIDDigitizerEventPosition
                           : (kIOHIDDigitizerEventRange | kIOHIDDigitizerEventTouch);
  CGPoint location = [touch locationInView:touch.window];
  IOHIDEventRef fingerEvent =
      api->createDigitizerFingerEvent(kCFAllocatorDefault, timestamp, 1, 2, eventMask, location.x,
                                      location.y, 0, 0, 0, touching, touching, 0);
  if (fingerEvent != NULL) {
    api->setIntegerValue(fingerEvent, kIOHIDEventFieldDigitizerIsDisplayIntegrated, 1);
    api->appendEvent(handEvent, fingerEvent, 0);
    CFRelease(fingerEvent);
  }
  return handEvent;
}

#pragma mark - UIKit SPI

// UIKit has no public API for injecting touches. The selectors below are the ones used by iOS
// testing frameworks such as KIF and EarlGrey; their availability is verified at runtime.
@interface UITouch (LynxEmulateTouch)
- (void)setWindow:(UIWindow*)window;
- (void)setView:(UIView*)view;
- (void)setTapCount:(NSUInteger)tapCount;
- (void)setIsTap:(BOOL)isTap;
- (void)setPhase:(UITouchPhase)phase;
- (void)setTimestamp:(NSTimeInterval)timestamp;
- (void)_setLocationInWindow:(CGPoint)location resetPrevious:(BOOL)resetPrevious;
- (void)_setIsFirstTouchForView:(BOOL)isFirstTouchForView;
- (void)_setHidEvent:(IOHIDEventRef)event;
@end

@interface UIEvent (LynxEmulateTouch)
- (void)_clearTouches;
- (void)_addTouch:(UITouch*)touch forDelayedDelivery:(BOOL)delayed;
- (void)_setHIDEvent:(IOHIDEventRef)event;
@end

@interface UIApplication (LynxEmulateTouch)
- (UIEvent*)_touchesEvent;
@end

static BOOL LynxIsTouchEmulationSupported(void) {
  static BOOL supported = NO;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    NSMutableArray<NSString*>* missing = [NSMutableArray array];
    NSArray<NSString*>* touchSelectors = @[
      @"setWindow:", @"setView:", @"setTapCount:", @"setPhase:", @"setTimestamp:",
      @"_setLocationInWindow:resetPrevious:", @"_setIsFirstTouchForView:", @"_setHidEvent:"
    ];
    for (NSString* name in touchSelectors) {
      if (![UITouch instancesRespondToSelector:NSSelectorFromString(name)]) {
        [missing addObject:[@"-[UITouch " stringByAppendingFormat:@"%@]", name]];
      }
    }
    Class touchesEventClass = NSClassFromString(@"UITouchesEvent") ?: [UIEvent class];
    NSArray<NSString*>* eventSelectors =
        @[ @"_clearTouches", @"_addTouch:forDelayedDelivery:", @"_setHIDEvent:" ];
    for (NSString* name in eventSelectors) {
      if (![touchesEventClass instancesRespondToSelector:NSSelectorFromString(name)]) {
        [missing addObject:[@"-[UIEvent " stringByAppendingFormat:@"%@]", name]];
      }
    }
    if (![UIApplication instancesRespondToSelector:@selector(_touchesEvent)]) {
      [missing addObject:@"-[UIApplication _touchesEvent]"];
    }
    if (missing.count > 0) {
      LLogError(@"LynxEmulateTouchHelper: touch emulation is unavailable, missing %@",
                [missing componentsJoinedByString:@", "]);
      return;
    }
    supported = LynxGetIOHIDEventApi() != NULL;
  });
  return supported;
}

#pragma mark - LynxEmulateTouchHelper

@interface LynxEmulateTouchHelper ()

@property(nonatomic, weak, nullable) LynxView* lynxView;
@property(nonatomic, assign) Boolean mouseWheelFlag;
@property(nonatomic, assign) CGPoint last;
@property(nonatomic, copy, nullable) dispatch_block_t task;
@property(nonatomic, strong, nullable) UITouch* touch;
@property(nonatomic, strong, nullable) UIEvent* event;

@end

@implementation LynxEmulateTouchHelper

- (nonnull instancetype)initWithLynxView:(LynxView*)view {
  if (self = [super init]) {
    _lynxView = view;
    _mouseWheelFlag = NO;
    _deltaScale = 5;
  }
  return self;
}

- (void)dealloc {
  [self cancelMouseWheelTask];
}

- (void)emulateTouch:(nonnull NSString*)type
         coordinateX:(int)x
         coordinateY:(int)y
              button:(nonnull NSString*)button
              deltaX:(CGFloat)dx
              deltaY:(CGFloat)dy
           modifiers:(int)modifiers
          clickCount:(int)click_count
      screenshotMode:(NSString*)screenshotMode {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf emulateTouch:type
                 coordinateX:x
                 coordinateY:y
                      button:button
                      deltaX:dx
                      deltaY:dy
                   modifiers:modifiers
                  clickCount:click_count
              screenshotMode:screenshotMode];
    });
    return;
  }

  LynxView* lynxView = _lynxView;
  if (lynxView == nil) {
    LLogError(@"LynxEmulateTouchHelper: lynxView is nil");
    return;
  }
  UIWindow* window = lynxView.window ?: [LynxUIKitAPIAdapter getKeyWindow];
  if (window == nil) {
    LLogError(@"LynxEmulateTouchHelper: no window is available to deliver %@", type);
    return;
  }
  CGPoint location = [self locationInWindow:window
                                   forPoint:CGPointMake(x, y)
                             screenshotMode:screenshotMode
                                   lynxView:lynxView];

  if ([type isEqualToString:kMouseEventPressed]) {
    [self handleMousePressedAtLocation:location inWindow:window clickCount:click_count];
  } else if ([type isEqualToString:kMouseEventMoved]) {
    [self handleMouseMovedToLocation:location];
  } else if ([type isEqualToString:kMouseEventReleased]) {
    [self handleMouseReleasedAtLocation:location];
  } else if ([type isEqualToString:kMouseEventWheel]) {
    [self handleMouseWheelAtLocation:location inWindow:window deltaX:dx deltaY:dy];
  } else {
    LLogWarn(@"LynxEmulateTouchHelper: unsupported event type %@", type);
  }
}

- (void)attachLynxView:(nonnull LynxView*)lynxView {
  [self cancelActiveTouch];
  _lynxView = lynxView;
}

#pragma mark - Mouse events

// CDP coordinates are in points relative to the screencast image, which is the LynxView in
// `lynxview` mode and the key window (i.e. the screen) in `fullscreen` mode.
- (CGPoint)locationInWindow:(UIWindow*)window
                   forPoint:(CGPoint)point
             screenshotMode:(NSString*)screenshotMode
                   lynxView:(LynxView*)lynxView {
  if ([screenshotMode isEqualToString:kScreenshotModeLynxView]) {
    return [lynxView convertPoint:point toView:window];
  }
  return [window convertPoint:point fromWindow:nil];
}

- (void)handleMousePressedAtLocation:(CGPoint)location
                            inWindow:(UIWindow*)window
                          clickCount:(int)clickCount {
  // A new press always starts a fresh touch sequence.
  [self cancelActiveTouch];
  [self beginTouchAtLocation:location inWindow:window tapCount:clickCount];
}

- (void)handleMouseMovedToLocation:(CGPoint)location {
  // Hovering without an active press has no touch counterpart.
  if (_touch == nil || _mouseWheelFlag) {
    return;
  }
  [self sendTouchWithPhase:UITouchPhaseMoved location:location];
}

- (void)handleMouseReleasedAtLocation:(CGPoint)location {
  if (_touch == nil || _mouseWheelFlag) {
    return;
  }
  [self sendTouchWithPhase:UITouchPhaseEnded location:location];
}

- (void)handleMouseWheelAtLocation:(CGPoint)location
                          inWindow:(UIWindow*)window
                            deltaX:(CGFloat)dx
                            deltaY:(CGFloat)dy {
  if (_touch != nil && !_mouseWheelFlag) {
    // Do not interfere with a drag driven by the mouse button.
    return;
  }
  [self cancelMouseWheelTask];
  if (_touch == nil) {
    [self beginTouchAtLocation:location inWindow:window tapCount:1];
    if (_touch == nil) {
      return;
    }
    _mouseWheelFlag = YES;
  }
  CGFloat scale = _deltaScale > 0 ? _deltaScale : 1;
  [self sendTouchWithPhase:UITouchPhaseMoved
                  location:CGPointMake(_last.x + dx / scale, _last.y + dy / scale)];

  __weak typeof(self) weakSelf = self;
  _task = dispatch_block_create(DISPATCH_BLOCK_INHERIT_QOS_CLASS, ^{
    [weakSelf stopMouseWheel];
  });
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, kMouseWheelReleaseDelayMs * NSEC_PER_MSEC),
                 dispatch_get_main_queue(), _task);
}

- (void)stopMouseWheel {
  _task = nil;
  if (_touch == nil || !_mouseWheelFlag) {
    return;
  }
  [self sendTouchWithPhase:UITouchPhaseEnded location:_last];
}

- (void)cancelMouseWheelTask {
  if (_task != nil) {
    dispatch_block_cancel(_task);
    _task = nil;
  }
}

// Ends an in-flight touch with a cancel so that gesture recognizers and scroll views that
// already received it do not stay in a began or changed state.
- (void)cancelActiveTouch {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf cancelActiveTouch];
    });
    return;
  }
  [self cancelMouseWheelTask];
  [self sendTouchWithPhase:UITouchPhaseCancelled location:_last];
}

#pragma mark - Touch synthesis

- (void)beginTouchAtLocation:(CGPoint)location inWindow:(UIWindow*)window tapCount:(int)tapCount {
  if (!LynxIsTouchEmulationSupported()) {
    return;
  }
  UITouch* touch = [[UITouch alloc] init];
  // The window has to be set first since it resets other state of the touch.
  [touch setWindow:window];
  [touch setTapCount:(NSUInteger)MAX(tapCount, 1)];
  if ([touch respondsToSelector:@selector(setIsTap:)]) {
    [touch setIsTap:YES];
  }
  [touch _setLocationInWindow:location resetPrevious:YES];
  [touch setView:[window hitTest:location withEvent:nil] ?: window];
  [touch setPhase:UITouchPhaseBegan];
  [touch _setIsFirstTouchForView:YES];
  [touch setTimestamp:[[NSProcessInfo processInfo] systemUptime]];

  _touch = touch;
  _last = location;
  [self sendCurrentTouch];
}

- (void)sendTouchWithPhase:(UITouchPhase)phase location:(CGPoint)location {
  if (_touch == nil) {
    return;
  }
  [_touch setTimestamp:[[NSProcessInfo processInfo] systemUptime]];
  [_touch _setLocationInWindow:location resetPrevious:NO];
  [_touch setPhase:phase];
  _last = location;
  [self sendCurrentTouch];
  if (phase == UITouchPhaseEnded || phase == UITouchPhaseCancelled) {
    _touch = nil;
    _event = nil;
    _mouseWheelFlag = NO;
  }
}

- (void)sendCurrentTouch {
  UIApplication* application = [UIApplication sharedApplication];
  UIEvent* event = [application _touchesEvent];
  if (event == nil) {
    // There is no running application to deliver through, e.g. in a standalone test runner.
    static BOOL logged = NO;
    if (!logged) {
      logged = YES;
      LLogError(@"LynxEmulateTouchHelper: failed to obtain the touches event");
    }
    return;
  }
  [event _clearTouches];
  IOHIDEventRef hidEvent = LynxCreateIOHIDEventForTouch(_touch);
  if (hidEvent != NULL) {
    [event _setHIDEvent:hidEvent];
    [_touch _setHidEvent:hidEvent];
  }
  [event _addTouch:_touch forDelayedDelivery:NO];
  _event = event;
  [application sendEvent:event];
  if (hidEvent != NULL) {
    CFRelease(hidEvent);
  }
}

@end
