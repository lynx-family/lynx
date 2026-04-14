// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"
#include <Carbon/Carbon.h>
#include <objc/objc.h>
#include <cctype>
#include "base/include/fml/memory/weak_ptr.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayMouseCursorPlugin.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayViewProvider_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterEmbedderKeyResponder.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterKeyPrimaryResponder.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterKeyboardManager.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterTextInputPlugin.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterView.h"

#include "clay/fml/logging.h"

namespace {

// Use different device ID for mouse and pan/zoom events, since we can't differentiate the actual
// device (mouse v.s. trackpad).
static constexpr int32_t kMousePointerDeviceId = 0;
static constexpr int32_t kPointerPanZoomDeviceId = 1;

using clay::KeyboardLayoutNotifier;
using clay::LayoutClue;

/// Clipboard plain text format.
constexpr char kTextPlainFormat[] = "text/plain";

/// The private notification for voice over.
static NSString* const EnhancedUserInterfaceNotification =
    @"NSApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification";
static NSString* const EnhancedUserInterfaceKey = @"AXEnhancedUserInterface";

/**
 * State tracking for mouse events, to adapt between the events coming from the system and the
 * events that the embedding API expects.
 */
struct MouseState {
  /**
   * The currently pressed buttons, as represented in FlutterPointerEvent.
   */
  int64_t buttons = 0;

  /**
   * The accumulated gesture pan.
   */
  CGFloat delta_x = 0;
  CGFloat delta_y = 0;

  /**
   * The accumulated gesture zoom scale.
   */
  CGFloat scale = 0;

  /**
   * The accumulated gesture rotation.
   */
  CGFloat rotation = 0;

  /**
   * Whether or not a kAdd event has been sent (or sent again since the last kRemove if tracking is
   * enabled). Used to determine whether to send a kAdd event before sending an incoming mouse
   * event, since Flutter expects pointers to be added before events are sent for them.
   */
  bool clay_state_is_added = false;

  /**
   * Whether or not a kDown has been sent since the last kAdd/kUp.
   */
  bool clay_state_is_down = false;

  /**
   * Whether or not mouseExited: was received while a button was down. Cocoa's behavior when
   * dragging out of a tracked area is to send an exit, then keep sending drag events until the last
   * button is released. Flutter doesn't expect to receive events after a kRemove, so the kRemove
   * for the exit needs to be delayed until after the last mouse button is released. If cursor
   * returns back to the window while still dragging, the flag is cleared in mouseEntered:.
   */
  bool has_pending_exit = false;

  /*
   * Whether or not a kPanZoomStart has been sent since the last kAdd/kPanZoomEnd.
   */
  bool flutter_state_is_pan_zoom_started = false;

  /**
   * State of pan gesture.
   */
  NSEventPhase pan_gesture_phase = NSEventPhaseNone;

  /**
   * State of scale gesture.
   */
  NSEventPhase scale_gesture_phase = NSEventPhaseNone;

  /**
   * State of rotate gesture.
   */
  NSEventPhase rotate_gesture_phase = NSEventPhaseNone;

  /**
   * Time of last scroll momentum event.
   */
  NSTimeInterval last_scroll_momentum_changed_time = 0;

  /**
   * Resets all gesture state to default values.
   */
  void GestureReset() {
    delta_x = 0;
    delta_y = 0;
    scale = 0;
    rotation = 0;
    flutter_state_is_pan_zoom_started = false;
    pan_gesture_phase = NSEventPhaseNone;
    scale_gesture_phase = NSEventPhaseNone;
    rotate_gesture_phase = NSEventPhaseNone;
  }

  /**
   * Resets all state to default values.
   */
  void Reset() {
    clay_state_is_added = false;
    clay_state_is_down = false;
    has_pending_exit = false;
    buttons = 0;
  }
};

/**
 * Returns the current Unicode layout data (kTISPropertyUnicodeKeyLayoutData).
 *
 * To use the returned data, convert it to CFDataRef first, finds its bytes
 * with CFDataGetBytePtr, then reinterpret it into const UCKeyboardLayout*.
 * It's returned in NSData* to enable auto reference count.
 */
NSData* currentKeyboardLayoutData() {
  TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
  CFTypeRef layout_data = TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
  if (layout_data == nil) {
    CFRelease(source);
    // TISGetInputSourceProperty returns null with Japanese keyboard layout.
    // Using TISCopyCurrentKeyboardLayoutInputSource to fix NULL return.
    // https://github.com/microsoft/node-native-keymap/blob/5f0699ded00179410a14c0e1b0e089fe4df8e130/src/keyboard_mac.mm#L91
    source = TISCopyCurrentKeyboardLayoutInputSource();
    layout_data = TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
  }
  return (__bridge_transfer NSData*)CFRetain(layout_data);
}

}  // namespace

#pragma mark - Private interface declaration.

/**
 * Private interface declaration for ClayViewProvider.
 */
@interface ClayViewProvider () <FlutterViewReshapeListener>

/**
 * The tracking area used to generate hover events, if enabled.
 */
@property(nonatomic) NSTrackingArea* trackingArea;

/**
 * The current state of the mouse and the sent mouse events.
 */
@property(nonatomic) MouseState mouseState;

/**
 * Event monitor for keyUp events.
 */
@property(nonatomic) id keyUpMonitor;

/**
 * TODO
 */
@property(nonatomic) FlutterKeyboardManager* keyboardManager;

@property(nonatomic) FlutterTextInputPlugin* textInputPlugin;

@property(nonatomic, nullable) FlutterView* view;

@property(nonatomic) KeyboardLayoutNotifier keyboardLayoutNotifier;

@property(nonatomic) NSData* keyboardLayoutData;

/**
 * Starts running |engine|, including any initial setup.
 */
- (BOOL)launchEngine;

/**
 * Updates |trackingArea| for the current tracking settings, creating it with
 * the correct mode if tracking is enabled, or removing it if not.
 */
- (void)configureTrackingArea;

/**
 * Creates and registers plugins used by this view controller.
 */
- (void)addInternalPlugins;

/**
 * Calls dispatchMouseEvent:phase: with a phase determined by self.mouseState.
 *
 * mouseState.buttons should be updated before calling this method.
 */
- (void)dispatchMouseEvent:(nonnull NSEvent*)event;

/**
 * Calls dispatchMouseEvent:phase: with a phase determined by event.phase.
 */
- (void)dispatchGestureEvent:(nonnull NSEvent*)event;

/**
 * Converts |event| to a ClayPointerEvent with the given phase, and sends it to the engine.
 */
- (void)dispatchMouseEvent:(nonnull NSEvent*)event phase:(ClayPointerPhase)phase;

/**
 * Initializes the KVO for user settings and passes the initial user settings to the engine.
 */
- (void)sendInitialSettings;

/**
 * Responds to updates in the user settings and passes this data to the engine.
 */
- (void)onSettingsChanged:(NSNotification*)notification;

/**
 * Plays a system sound. |soundType| specifies which system sound to play. Valid
 * values can be found in the SystemSoundType enum in the services SDK package.
 */
- (void)playSystemSound:(NSString*)soundType;

/**
 * Reads the data from the clipboard. |format| specifies the media type of the
 * data to obtain.
 */
- (NSDictionary*)getClipboardData:(NSString*)format;

/**
 * Clears contents and writes new data into clipboard. |data| is a dictionary where
 * the keys are the type of data, and tervalue the data to be stored.
 */
- (void)setClipboardData:(NSDictionary*)data;

/**
 * Returns true iff the clipboard contains nonempty string data.
 */
- (BOOL)clipboardHasStrings;

@end

#pragma mark - ClayViewProvider implementation.

@implementation ClayViewProvider {
  ClayServiceManager* _serviceManager;

  void (*_ime_callback)(NSEvent*, void*);
  void* _ime_callback_arg;

  BOOL _viewDidLoad;
  BOOL _isLaunchEngineSuccess;
}

/**
 * Performs initialization that's common between the different init paths.
 */
static void CommonInit(ClayViewProvider* controller) {
  if (!controller->_engine) {
    controller->_engine = [[FlutterEngine alloc] initWithName:@"lynx.clay"
                                       allowHeadlessExecution:NO];
    [controller->_engine setViewProvider:controller];
  }
  controller->_mouseTrackingMode = FlutterMouseTrackingModeInKeyWindow;
}

- (instancetype)init {
  self = [super init];
  NSAssert(self, @"Super init cannot be nil");
  _ime_callback = nullptr;
  _viewDidLoad = NO;
  _serviceManager = [[ClayServiceManager alloc] init];
  CommonInit(self);
  _isLaunchEngineSuccess = [self launchEngine];
  return self;
}

- (FlutterView*)view {
  if (!_view) {
    [self loadView];
  }
  return _view;
}

- (void)loadView {
  FlutterView* flutterView;
  id<MTLDevice> device = _engine.renderer.device;
  id<MTLCommandQueue> commandQueue = _engine.renderer.commandQueue;
  if (!device || !commandQueue) {
    FML_LOG(ERROR) << "Unable to create FlutterView; no MTLDevice or MTLCommandQueue available.";
    return;
  }
  flutterView = [[FlutterView alloc] initWithMTLDevice:device
                                          commandQueue:commandQueue
                                       reshapeListener:self];

  flutterView.provider = self;
  self.view = flutterView;
  _viewDidLoad = YES;
  // As viewWillAppear not be called, put it here.
  [self listenForMetaModifiedKeyUpEvents];

  // track mouse hover events
  [self configureTrackingArea];
}

- (void)viewDidLoad {
  [self configureTrackingArea];
}

- (void)viewWillAppear {
  if (!_engine.running) {
    [self launchEngine];
  }
  [self listenForMetaModifiedKeyUpEvents];
}

- (void)viewWillDisappear {
  // Per Apple's documentation, it is discouraged to call removeMonitor: in dealloc, and it's
  // recommended to be called earlier in the lifecycle.
  [NSEvent removeMonitor:_keyUpMonitor];
  _keyUpMonitor = nil;
}

- (void)onEnterForeground {
  if (!_engine) {
    return;
  }
  [_engine onEnterForeground];
}

- (void)onEnterBackground {
  if (!_engine) {
    return;
  }
  [_engine onEnterBackground];
}

- (void)setVisible:(BOOL)visible {
  if (!_engine) {
    return;
  }
  [_engine setVisible:visible];
  if (_view) {
    // Set the synchronizer's visibility to avoid blocking when resizing.
    [[_view threadSynchronizer] setVisible:visible];
  }
}

- (void)setInterceptUrlCallback:(FlutterViewShouldInterceptUrlCallback _Nullable)callback {
  if (!_engine) {
    return;
  }

  [_engine setInterceptUrlCallback:callback];
}

- (void)dealloc {
  _engine.view = nil;
  self.view.provider = nil;
}

- (nullable void*)clayViewContext {
  if (!_engine) {
    return nullptr;
  }

  void* result = [_engine clayViewContext];
  return result;
}

#pragma mark - Public methods

- (void)setMouseTrackingMode:(FlutterMouseTrackingMode)mode {
  if (_mouseTrackingMode == mode) {
    return;
  }
  _mouseTrackingMode = mode;
  [self configureTrackingArea];
}

#pragma mark - Framework-internal methods

- (FlutterView*)flutterView {
  return self.view;
}

#pragma mark - Private methods

- (BOOL)launchEngine {
  // Register internal plugins before starting the engine.
  [self addInternalPlugins];
  _engine.view = self.view;
  if (![_engine runWithEntrypoint:nil]) {
    return NO;
  }
  // Send the initial user settings such as brightness and text scale factor
  // to the engine.
  // TODO(stuartmorgan): Move this logic to FlutterEngine.
  [self sendInitialSettings];
  return YES;
}

// macOS does not call keyUp: on a key while the command key is pressed. This results in a loss
// of a key event once the modified key is released. This method registers the
// ViewController as a listener for a keyUp event before it's handled by NSApplication, and should
// NOT modify the event to avoid any unexpected behavior.
- (void)listenForMetaModifiedKeyUpEvents {
  if (_keyUpMonitor != nil) {
    // It is possible for [NSViewController viewWillAppear] to be invoked multiple times
    // in a row. https://github.com/flutter/flutter/issues/105963
    return;
  }
  ClayViewProvider* __weak weakSelf = self;
  _keyUpMonitor = [NSEvent
      addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp
                                   handler:^NSEvent*(NSEvent* event) {
                                     // Intercept keyUp only for events triggered on the current
                                     // view.
                                     if (weakSelf.view && weakSelf.view.superview &&
                                         ([[event window] firstResponder] == weakSelf.view) &&
                                         ([event modifierFlags] & NSEventModifierFlagCommand) &&
                                         ([event type] == NSEventTypeKeyUp))
                                       [weakSelf keyUp:event];
                                     return event;
                                   }];
}

- (void)configureTrackingArea {
  if (_mouseTrackingMode != FlutterMouseTrackingModeNone && self.view) {
    NSTrackingAreaOptions options =
        NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingInVisibleRect;
    switch (_mouseTrackingMode) {
      case FlutterMouseTrackingModeInKeyWindow:
        options |= NSTrackingActiveInKeyWindow;
        break;
      case FlutterMouseTrackingModeInActiveApp:
        options |= NSTrackingActiveInActiveApp;
        break;
      case FlutterMouseTrackingModeAlways:
        options |= NSTrackingActiveAlways;
        break;
      default:
        FML_LOG(ERROR) << "Error: Unrecognized mouse tracking mode: " << _mouseTrackingMode;
        return;
    }
    _trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                 options:options
                                                   owner:self
                                                userInfo:nil];
    [self.view addTrackingArea:_trackingArea];
  } else if (_trackingArea) {
    [self.view removeTrackingArea:_trackingArea];
    _trackingArea = nil;
  }
}

- (void)addInternalPlugins {
  __weak ClayViewProvider* weakSelf = self;
  _keyboardManager = [[FlutterKeyboardManager alloc] initWithViewDelegate:weakSelf];
  _textInputPlugin = [[FlutterTextInputPlugin alloc] initWithClayProvider:self];
  _mouseCursorPlugin = [[ClayMouseCursorPlugin alloc] init];
}

- (void)dispatchMouseEvent:(nonnull NSEvent*)event {
  ClayPointerPhase phase =
      _mouseState.buttons == 0
          ? (_mouseState.clay_state_is_down ? kClayPointerPhaseUp : kClayPointerPhaseHover)
          : (_mouseState.clay_state_is_down ? kClayPointerPhaseMove : kClayPointerPhaseDown);
  [self dispatchMouseEvent:event phase:phase];
}

- (void)dispatchGestureEvent:(nonnull NSEvent*)event {
  if (event.phase == NSEventPhaseMayBegin) {
    return;
  }
  if (event.phase == NSEventPhaseBegan) {
    [self dispatchMouseEvent:event phase:kClayPointerPhasePanZoomStart];
  } else if (event.phase == NSEventPhaseChanged) {
    [self dispatchMouseEvent:event phase:kClayPointerPhasePanZoomUpdate];
  } else if (event.phase == NSEventPhaseEnded || event.phase == NSEventPhaseCancelled) {
    [self dispatchMouseEvent:event phase:kClayPointerPhasePanZoomEnd];
  } else if (event.phase == NSEventPhaseNone && event.momentumPhase == NSEventPhaseNone) {
    [self dispatchMouseEvent:event phase:kClayPointerPhaseHover];
  } else {
    // Waiting until the first momentum change event is a workaround for an issue where
    // touchesBegan: is called unexpectedly while in low power mode within the interval between
    // momentum start and the first momentum change.
    if (event.momentumPhase == NSEventPhaseChanged) {
      _mouseState.last_scroll_momentum_changed_time = event.timestamp;
    }
    // Skip momentum update events, the framework will generate scroll momentum.
    NSAssert(event.momentumPhase != NSEventPhaseNone,
             @"Received gesture event with unexpected phase");
  }
}

- (void)dispatchMouseEvent:(NSEvent*)event phase:(ClayPointerPhase)phase {
  // There are edge cases where the system will deliver enter out of order relative to other
  // events (e.g., drag out and back in, release, then click; mouseDown: will be called before
  // mouseEntered:). Discard those events, since the add will already have been synthesized.
  if (_mouseState.clay_state_is_added && phase == kClayPointerPhaseAdd) {
    return;
  }

  // Multiple gesture recognizers could be active at once, we can't send multiple kPanZoomStart.
  // For example: rotation and magnification.
  if (phase == kClayPointerPhasePanZoomStart || phase == kClayPointerPhasePanZoomEnd) {
    if (event.type == NSEventTypeScrollWheel) {
      _mouseState.pan_gesture_phase = event.phase;
    } else if (event.type == NSEventTypeMagnify) {
      _mouseState.scale_gesture_phase = event.phase;
    } else if (event.type == NSEventTypeRotate) {
      _mouseState.rotate_gesture_phase = event.phase;
    }
  }
  if (phase == kClayPointerPhasePanZoomStart) {
    if (event.type == NSEventTypeScrollWheel) {
      // Ensure scroll inertia cancel event is not sent afterwards.
      _mouseState.last_scroll_momentum_changed_time = 0;
    }
    if (_mouseState.flutter_state_is_pan_zoom_started) {
      // Already started on a previous gesture type
      return;
    }
    _mouseState.flutter_state_is_pan_zoom_started = true;
  }
  if (phase == kClayPointerPhasePanZoomEnd) {
    if (!_mouseState.flutter_state_is_pan_zoom_started) {
      // NSEventPhaseCancelled is sometimes received at incorrect times in the state
      // machine, just ignore it here if it doesn't make sense
      // (we have no active gesture to cancel).
      NSAssert(event.phase == NSEventPhaseCancelled,
               @"Received gesture event with unexpected phase");
      return;
    }
    // NSEventPhase values are powers of two, we can use this to inspect merged phases.
    NSEventPhase all_gestures_fields = _mouseState.pan_gesture_phase |
                                       _mouseState.scale_gesture_phase |
                                       _mouseState.rotate_gesture_phase;
    NSEventPhase active_mask = NSEventPhaseBegan | NSEventPhaseChanged;
    if ((all_gestures_fields & active_mask) != 0) {
      // Even though this gesture type ended, a different type is still active.
      return;
    }
  }

  // If a pointer added event hasn't been sent, synthesize one using this event for the basic
  // information.
  if (!_mouseState.clay_state_is_added && phase != kClayPointerPhaseAdd) {
    // Only the values extracted for use in clayEvent below matter, the rest are dummy values.
    NSEvent* addEvent = [NSEvent enterExitEventWithType:NSEventTypeMouseEntered
                                               location:event.locationInWindow
                                          modifierFlags:0
                                              timestamp:event.timestamp
                                           windowNumber:event.windowNumber
                                                context:nil
                                            eventNumber:0
                                         trackingNumber:0
                                               userData:NULL];
    [self dispatchMouseEvent:addEvent phase:kClayPointerPhaseAdd];
  }

  NSPoint locationInView = [self.view convertPoint:event.locationInWindow fromView:nil];
  NSPoint locationInBackingCoordinates = [self.view convertPointToBacking:locationInView];
  int32_t device = kMousePointerDeviceId;
  ClayPointerDeviceKind deviceKind = kClayPointerDeviceKindMouse;
  if (phase == kClayPointerPhasePanZoomStart || phase == kClayPointerPhasePanZoomUpdate ||
      phase == kClayPointerPhasePanZoomEnd) {
    device = kPointerPanZoomDeviceId;
    deviceKind = kClayPointerDeviceKindTrackpad;
  }
  ClayPointerEvent clayEvent = {
      .struct_size = sizeof(clayEvent),
      .phase = phase,
      .timestamp = static_cast<size_t>(event.timestamp * USEC_PER_SEC),
      .x = locationInBackingCoordinates.x,
      .y = -locationInBackingCoordinates.y,  // convertPointToBacking makes this negative.
      .device = device,
      .device_kind = deviceKind,
      // If a click triggered a synthesized kAdd, don't pass the buttons in that event.
      .buttons = phase == kClayPointerPhaseAdd ? 0 : _mouseState.buttons,
      .is_precise_scroll = 1,
  };

  if (phase == kClayPointerPhasePanZoomUpdate) {
    if (event.type == NSEventTypeScrollWheel) {
      _mouseState.delta_x += event.scrollingDeltaX * self.view.layer.contentsScale;
      _mouseState.delta_y += event.scrollingDeltaY * self.view.layer.contentsScale;
    } else if (event.type == NSEventTypeMagnify) {
      _mouseState.scale += event.magnification;
    } else if (event.type == NSEventTypeRotate) {
      _mouseState.rotation += event.rotation * (-M_PI / 180.0);
    }
    clayEvent.pan_x = _mouseState.delta_x;
    clayEvent.pan_y = _mouseState.delta_y;
    // Scale value needs to be normalized to range 0->infinity.
    clayEvent.scale = pow(2.0, _mouseState.scale);
    clayEvent.rotation = _mouseState.rotation;
  } else if (phase == kClayPointerPhasePanZoomEnd) {
    _mouseState.GestureReset();
  } else if (phase != kClayPointerPhasePanZoomStart && event.type == NSEventTypeScrollWheel) {
    clayEvent.signal_kind = kClayPointerSignalKindScroll;

    double pixelsPerLine = 1.0;
    if (!event.hasPreciseScrollingDeltas) {
      CGEventSourceRef source = CGEventCreateSourceFromEvent(event.CGEvent);
      pixelsPerLine = CGEventSourceGetPixelsPerLine(source);
      if (source) {
        CFRelease(source);
      }
      clayEvent.is_precise_scroll = 0;
    }
    double scaleFactor = self.view.layer.contentsScale;
    clayEvent.scroll_delta_x = -event.scrollingDeltaX * pixelsPerLine * scaleFactor;
    clayEvent.scroll_delta_y = -event.scrollingDeltaY * pixelsPerLine * scaleFactor;
  }
  [_engine sendPointerEvent:clayEvent];

  // Update tracking of state as reported to Flutter.
  if (phase == kClayPointerPhaseDown) {
    _mouseState.clay_state_is_down = true;
  } else if (phase == kClayPointerPhaseUp) {
    _mouseState.clay_state_is_down = false;
    if (_mouseState.has_pending_exit) {
      [self dispatchMouseEvent:event phase:kClayPointerPhaseRemove];
      _mouseState.has_pending_exit = false;
    }
  } else if (phase == kClayPointerPhaseAdd) {
    _mouseState.clay_state_is_added = true;
  } else if (phase == kClayPointerPhaseRemove) {
    _mouseState.Reset();
  }
}

- (void)onSettingsChanged:(NSNotification*)notification {
  // TODO(jonahwilliams): https://github.com/flutter/flutter/issues/32015.
}

- (void)sendInitialSettings {
  // TODO(jonahwilliams): https://github.com/flutter/flutter/issues/32015.
  [[NSDistributedNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(onSettingsChanged:)
             name:@"AppleInterfaceThemeChangedNotification"
           object:nil];
  [self onSettingsChanged:nil];
}

- (void)playSystemSound:(NSString*)soundType {
  if ([soundType isEqualToString:@"SystemSoundType.alert"]) {
    NSBeep();
  }
}

- (NSDictionary*)getClipboardData:(NSString*)format {
  NSPasteboard* pasteboard = self.pasteboard;
  if ([format isEqualToString:@(kTextPlainFormat)]) {
    NSString* stringInPasteboard = [pasteboard stringForType:NSPasteboardTypeString];
    return stringInPasteboard == nil ? nil : @{@"text" : stringInPasteboard};
  }
  return nil;
}

- (void)setClipboardData:(NSDictionary*)data {
  NSPasteboard* pasteboard = self.pasteboard;
  NSString* text = data[@"text"];
  [pasteboard clearContents];
  if (text && ![text isEqual:[NSNull null]]) {
    [pasteboard setString:text forType:NSPasteboardTypeString];
  }
}

- (BOOL)clipboardHasStrings {
  return [self.pasteboard stringForType:NSPasteboardTypeString].length > 0;
}

- (NSPasteboard*)pasteboard {
  return [NSPasteboard generalPasteboard];
}

- (BOOL)viewLoaded {
  return _viewDidLoad;
}

- (BOOL)isLaunchEngineSuccess {
  return _isLaunchEngineSuccess;
}

#pragma mark - FlutterViewReshapeListener

/**
 * Responds to view reshape by notifying the engine of the change in dimensions.
 */
- (void)viewDidReshape:(NSView*)view {
  [_engine updateWindowMetrics];
}

#pragma mark - FlutterPluginRegistry

- (id<FlutterPluginRegistrar>)registrarForPlugin:(NSString*)pluginName {
  return [_engine registrarForPlugin:pluginName];
}

#pragma mark - Use Clay Service

- (ClayServiceManager*)GetClayServiceManager {
  return _serviceManager;
}

#pragma mark - NSResponder

- (BOOL)acceptsFirstResponder {
  return NO;
}

- (void)keyDown:(NSEvent*)event {
  if (_ime_callback) {
    _ime_callback(event, _ime_callback_arg);
    return;
  }
  if ([_textInputPlugin isComposing] && [_textInputPlugin handleKeyEvent:event]) {
    return;
  }
  [_keyboardManager handleEvent:event];
}

- (void)keyUp:(NSEvent*)event {
  if (_ime_callback) {
    _ime_callback(event, _ime_callback_arg);
    return;
  }
  if ([_textInputPlugin isComposing] && [_textInputPlugin handleKeyEvent:event]) {
    return;
  }
  [_keyboardManager handleEvent:event];
}

- (void)requestIME:(nullable void*)callback arg:(nullable void*)arg {
  _ime_callback = (void (*)(NSEvent*, void*))callback;
  _ime_callback_arg = arg;
}

/**
 * Returns YES if provided event is being currently redispatched by keyboard manager.
 */
- (BOOL)isDispatchingKeyEvent:(nonnull NSEvent*)event {
  return [_keyboardManager isDispatchingKeyEvent:event];
}

- (void)flagsChanged:(NSEvent*)event {
  if (_ime_callback) {
    _ime_callback(event, _ime_callback_arg);
    return;
  }
  if ([_textInputPlugin isComposing] && [_textInputPlugin handleKeyEvent:event]) {
    return;
  }
  [_keyboardManager handleEvent:event];
}

- (void)mouseEntered:(NSEvent*)event {
  [self dispatchMouseEvent:event phase:kClayPointerPhaseAdd];
}

- (void)mouseExited:(NSEvent*)event {
  if (_mouseState.buttons != 0) {
    _mouseState.has_pending_exit = true;
    return;
  }
  [self dispatchMouseEvent:event phase:kClayPointerPhaseRemove];
}

- (void)mouseDown:(NSEvent*)event {
  _mouseState.buttons |= kClayPointerMouseButtonsMousePrimary;
  [self dispatchMouseEvent:event];
}

- (void)mouseUp:(NSEvent*)event {
  _mouseState.buttons &= ~static_cast<uint64_t>(kClayPointerMouseButtonsMousePrimary);
  [self dispatchMouseEvent:event];
}

- (void)mouseDragged:(NSEvent*)event {
  [self dispatchMouseEvent:event];
}

- (void)rightMouseDown:(NSEvent*)event {
  _mouseState.buttons |= kClayPointerMouseButtonsMouseSecondary;
  [self dispatchMouseEvent:event];
}

- (void)rightMouseUp:(NSEvent*)event {
  _mouseState.buttons &= ~static_cast<uint64_t>(kClayPointerMouseButtonsMouseSecondary);
  [self dispatchMouseEvent:event];
}

- (void)rightMouseDragged:(NSEvent*)event {
  [self dispatchMouseEvent:event];
}

- (void)otherMouseDown:(NSEvent*)event {
  _mouseState.buttons |= (1 << event.buttonNumber);
  [self dispatchMouseEvent:event];
}

- (void)otherMouseUp:(NSEvent*)event {
  _mouseState.buttons &= ~static_cast<uint64_t>(1 << event.buttonNumber);
  [self dispatchMouseEvent:event];
}

- (void)otherMouseDragged:(NSEvent*)event {
  [self dispatchMouseEvent:event];
}

- (void)mouseMoved:(NSEvent*)event {
  [self dispatchMouseEvent:event];
}

- (void)scrollWheel:(NSEvent*)event {
  [self dispatchGestureEvent:event];
}

- (void)magnifyWithEvent:(NSEvent*)event {
  [self dispatchGestureEvent:event];
}

#pragma mark - FlutterKeyboardViewDelegate
- (void)sendKeyEvent:(const ClayKeyEvent&)event
            callback:(nullable ClayKeyEventCallback)callback
            userData:(nullable void*)userData {
  [_engine sendKeyEvent:event callback:callback userData:userData];
}

- (BOOL)onTextInputKeyEvent:(nonnull NSEvent*)event {
  return [_textInputPlugin handleKeyEvent:event];
}

- (void)subscribeToKeyboardLayoutChange:(nullable KeyboardLayoutNotifier)callback {
  _keyboardLayoutNotifier = callback;
}

- (LayoutClue)lookUpLayoutForKeyCode:(uint16_t)keyCode shift:(BOOL)shift {
  if (_keyboardLayoutData == nil) {
    _keyboardLayoutData = currentKeyboardLayoutData();
  }
  const UCKeyboardLayout* layout = reinterpret_cast<const UCKeyboardLayout*>(
      CFDataGetBytePtr((__bridge CFDataRef)_keyboardLayoutData));

  UInt32 deadKeyState = 0;
  UniCharCount stringLength = 0;
  UniChar resultChar;

  UInt32 modifierState = ((shift ? shiftKey : 0) >> 8) & 0xFF;
  UInt32 keyboardType = LMGetKbdLast();

  bool isDeadKey = false;
  OSStatus status =
      UCKeyTranslate(layout, keyCode, kUCKeyActionDown, modifierState, keyboardType,
                     kUCKeyTranslateNoDeadKeysBit, &deadKeyState, 1, &stringLength, &resultChar);
  // For dead keys, press the same key again to get the printable representation of the key.
  if (status == noErr && stringLength == 0 && deadKeyState != 0) {
    isDeadKey = true;
    status =
        UCKeyTranslate(layout, keyCode, kUCKeyActionDown, modifierState, keyboardType,
                       kUCKeyTranslateNoDeadKeysBit, &deadKeyState, 1, &stringLength, &resultChar);
  }

  if (status == noErr && stringLength == 1 && !std::iscntrl(resultChar)) {
    return LayoutClue{resultChar, isDeadKey};
  }
  return LayoutClue{0, false};
}

- (void)performMouseDragLeave {
  if (_engine) {
    [_engine performMouseDragLeave];
  }
}

- (void)performMouseDragEnterAndOverAtPoint:(NSPoint)point {
  if (_engine) {
    [_engine performMouseDragEnterAndOverAtPoint:point];
  }
}

- (void)performMouseDragDropAtPoint:(NSPoint)point
                               type:(NSString* _Nonnull)type
                        dropContent:(id _Nonnull)content {
  if (_engine) {
    [_engine performMouseDragDropAtPoint:point type:type dropContent:content];
  }
}

@end
