// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AppKit/AppKit.h>

#include <functional>
#include <limits>
#include <memory>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "devtool/lynx_devtool/input/synthetic_gesture_controller.h"
#include "devtool/lynx_devtool/input/synthetic_tap_gesture.h"
#include "platform/darwin/macos/lynx_devtool/macos_input_event_target.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

@interface TapTestEventView : NSView
@property(nonatomic, strong) NSEvent* down;
@property(nonatomic, strong) NSEvent* up;
@end
@implementation TapTestEventView
- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return YES;
}
- (void)mouseDown:(NSEvent*)event {
  self.down = event;
}
- (void)mouseUp:(NSEvent*)event {
  self.up = event;
}
@end

@interface TapTestFlippedView : TapTestEventView
@end
@implementation TapTestFlippedView
- (BOOL)isFlipped {
  return YES;
}
@end

@interface TapTestAction : NSObject
@property(nonatomic) NSInteger count;
- (void)clicked:(id)sender;
@end
@implementation TapTestAction
- (void)clicked:(id)sender {
  self.count++;
}
@end

namespace lynx {
namespace devtool {
namespace {

constexpr NSEventMask kMouseEvents =
    NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged;

class MacOSInputEventTargetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(NSThread.isMainThread);
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    [NSApp finishLaunching];
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    window_ = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 400, 300)
                                          styleMask:NSWindowStyleMaskTitled
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    window_.releasedWhenClosed = NO;
    window_.animationBehavior = NSWindowAnimationBehaviorNone;
    host_ = [[NSView alloc] initWithFrame:NSMakeRect(60, 80, 120, 100)];
    [window_.contentView addSubview:host_];
    [window_ orderFront:nil];
    ProcessWindowEvents();
    target_ = std::make_shared<MacOSInputEventTarget>((__bridge void*)host_);
    Drain();
  }

  void TearDown() override {
    target_->Invalidate();
    Drain();
    [window_ close];
    target_.reset();
    host_ = nil;
    window_ = nil;
  }

  input::PointerEvent Event(input::PointerEventType type, float x = 280.5f, float y = 95.25f,
                            int id = 7) {
    input::PointerEvent event;
    event.source_type = input::PointerSourceType::kMouse;
    event.type = type;
    event.action_pointer_id = id;
    event.click_count = 1;
    event.timestamp_us = static_cast<int64_t>(NSProcessInfo.processInfo.systemUptime * 1000000);
    event.buttons = type == input::PointerEventType::kDown ? input::kPrimaryButton : 0;
    input::Pointer pointer;
    pointer.id = id;
    pointer.x = x;
    pointer.y = y;
    event.pointers.push_back(pointer);
    return event;
  }

  NSEvent* NextMouse() {
    return [NSApp nextEventMatchingMask:kMouseEvents
                              untilDate:NSDate.distantPast
                                 inMode:NSDefaultRunLoopMode
                                dequeue:YES];
  }

  void Drain() {
    while (NextMouse()) {
    }
  }

  void ProcessWindowEvents() {
    // Give the run loop a turn to register window geometry with AppKit before
    // converting native events. A distant-past deadline only drains the queue.
    while (NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                               untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]
                                                  inMode:NSDefaultRunLoopMode
                                                 dequeue:YES]) {
      [NSApp sendEvent:event];
    }
  }

  bool PumpUntil(const std::function<bool()>& done) {
    NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:2];
    while (!done() && deadline.timeIntervalSinceNow > 0) {
      NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES];
      if (event) {
        [NSApp sendEvent:event];
      }
    }
    return done();
  }

  NSButton* AddButton(TapTestAction* action) {
    NSButton* button = [[NSButton alloc] initWithFrame:NSMakeRect(230, 190, 100, 30)];
    button.title = @"Native target";
    button.bezelStyle = NSBezelStyleRounded;
    button.target = action;
    button.action = @selector(clicked:);
    [window_.contentView addSubview:button];
    [window_ orderFront:nil];
    return button;
  }

  NSWindow* window_;
  NSView* host_;
  std::shared_ptr<MacOSInputEventTarget> target_;
};

TEST_F(MacOSInputEventTargetTest, AdvertisesOnlyMouseForAttachedWindow) {
  auto capabilities = target_->GetPointerCapabilities();
  EXPECT_EQ(capabilities.default_source_type, input::PointerSourceType::kMouse);
  EXPECT_TRUE(capabilities.supports_mouse);
  EXPECT_FALSE(capabilities.supports_touch);
  [host_ removeFromSuperview];
  EXPECT_FALSE(target_->GetPointerCapabilities().supports_mouse);
}

TEST_F(MacOSInputEventTargetTest, UnorderedWindowDoesNotAcceptMouseInput) {
  [window_ orderOut:nil];
  EXPECT_FALSE(target_->GetPointerCapabilities().supports_mouse);
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
  EXPECT_EQ(NextMouse(), nil);
}

TEST_F(MacOSInputEventTargetTest, DoesNotRetainTheHostView) {
  __weak NSView* weak_host;
  @autoreleasepool {
    NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    weak_host = view;
    target_ = std::make_shared<MacOSInputEventTarget>((__bridge void*)view);
  }
  EXPECT_EQ(weak_host, nil);
  EXPECT_FALSE(target_->GetPointerCapabilities().supports_mouse);
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
}

TEST_F(MacOSInputEventTargetTest, UsesWindowContentPointsWithFractionalCoordinates) {
  TapTestEventView* receiver = [[TapTestEventView alloc] initWithFrame:window_.contentView.bounds];
  window_.contentView = receiver;
  [receiver addSubview:host_];
  ProcessWindowEvents();
  const auto pointer_down = Event(input::PointerEventType::kDown);
  ASSERT_TRUE(target_->InjectPointerEvent(pointer_down));
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kUp)));
  ASSERT_TRUE(PumpUntil([&]() { return receiver.down && receiver.up; }));
  NSEvent* down = receiver.down;
  ASSERT_NE(down, nil);
  EXPECT_EQ(down.type, NSEventTypeLeftMouseDown);
  EXPECT_EQ(down.windowNumber, window_.windowNumber);
  ASSERT_EQ(down.window, window_);
  EXPECT_DOUBLE_EQ(down.locationInWindow.x, 280.5);
  EXPECT_DOUBLE_EQ(down.locationInWindow.y, 204.75);
  EXPECT_DOUBLE_EQ(down.timestamp, pointer_down.timestamp_us / 1000000.0);
  EXPECT_EQ(down.clickCount, 1);
  NSEvent* up = receiver.up;
  ASSERT_NE(up, nil);
  ASSERT_EQ(up.window, window_);
  EXPECT_EQ(up.type, NSEventTypeLeftMouseUp);
  EXPECT_DOUBLE_EQ(up.locationInWindow.x, down.locationInWindow.x);
  EXPECT_DOUBLE_EQ(up.locationInWindow.y, down.locationInWindow.y);
  EXPECT_EQ(NextMouse(), nil);
}

TEST_F(MacOSInputEventTargetTest, ConvertsFlippedContentViewToWindowBaseSpace) {
  TapTestFlippedView* receiver =
      [[TapTestFlippedView alloc] initWithFrame:NSMakeRect(0, 0, 400, 300)];
  window_.contentView = receiver;
  [window_.contentView addSubview:host_];
  ProcessWindowEvents();
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kUp)));
  ASSERT_TRUE(PumpUntil([&]() { return receiver.down && receiver.up; }));
  NSEvent* down = receiver.down;
  ASSERT_NE(down, nil);
  ASSERT_EQ(down.window, window_);
  EXPECT_DOUBLE_EQ(down.locationInWindow.x, 280.5);
  EXPECT_DOUBLE_EQ(down.locationInWindow.y, 204.75);
}

TEST_F(MacOSInputEventTargetTest, RejectsInvalidSourceCoordinatesAndPointerSequence) {
  auto event = Event(input::PointerEventType::kDown);
  event.source_type = input::PointerSourceType::kTouch;
  EXPECT_FALSE(target_->InjectPointerEvent(event));
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown, -1, 0)));
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown, 400, 0)));
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown, 0, 300)));
  EXPECT_FALSE(target_->InjectPointerEvent(
      Event(input::PointerEventType::kDown, std::numeric_limits<float>::infinity(), 0)));
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kUp)));
  event = Event(input::PointerEventType::kDown);
  event.action_pointer_id = 42;
  EXPECT_FALSE(target_->InjectPointerEvent(event));
  event = Event(input::PointerEventType::kDown);
  event.pointers.push_back(event.pointers[0]);
  EXPECT_FALSE(target_->InjectPointerEvent(event));
  EXPECT_EQ(NextMouse(), nil);
}

TEST_F(MacOSInputEventTargetTest, CancelsOriginalWindowWhenHostIsDetached) {
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
  ASSERT_NE(NextMouse(), nil);
  [host_ removeFromSuperview];
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kUp)));
  NSEvent* dragged = NextMouse();
  NSEvent* released = NextMouse();
  ASSERT_NE(dragged, nil);
  ASSERT_NE(released, nil);
  EXPECT_EQ(dragged.type, NSEventTypeLeftMouseDragged);
  EXPECT_EQ(released.type, NSEventTypeLeftMouseUp);
  EXPECT_EQ(released.windowNumber, window_.windowNumber);
  EXPECT_LT(released.locationInWindow.x, 0);
  EXPECT_LT(released.locationInWindow.y, 0);
  EXPECT_EQ(NextMouse(), nil);
}

TEST_F(MacOSInputEventTargetTest, InvalidationReleasesOnceAndRejectsFurtherInput) {
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
  ASSERT_NE(NextMouse(), nil);
  target_->Invalidate();
  ASSERT_NE(NextMouse(), nil);
  ASSERT_NE(NextMouse(), nil);
  target_->Invalidate();
  EXPECT_EQ(NextMouse(), nil);
  EXPECT_FALSE(target_->GetPointerCapabilities().supports_mouse);
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
}

TEST_F(MacOSInputEventTargetTest, ReparentingCancelsTheOriginalWindow) {
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
  ASSERT_NE(NextMouse(), nil);
  NSWindow* other = [[NSWindow alloc] initWithContentRect:NSMakeRect(600, 100, 400, 300)
                                                styleMask:NSWindowStyleMaskTitled
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
  other.releasedWhenClosed = NO;
  [other.contentView addSubview:host_];
  [other orderFront:nil];
  EXPECT_FALSE(target_->InjectPointerEvent(Event(input::PointerEventType::kUp)));
  NSEvent* dragged = NextMouse();
  NSEvent* released = NextMouse();
  EXPECT_EQ(dragged.type, NSEventTypeLeftMouseDragged);
  EXPECT_EQ(released.type, NSEventTypeLeftMouseUp);
  EXPECT_EQ(released.windowNumber, window_.windowNumber);
  [other close];
}

TEST_F(MacOSInputEventTargetTest, ClosingWindowCancelsPressedPointer) {
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown)));
  ASSERT_NE(NextMouse(), nil);
  [window_ close];
  EXPECT_FALSE(target_->GetPointerCapabilities().supports_mouse);
  NSEvent* dragged = NextMouse();
  NSEvent* released = NextMouse();
  ASSERT_NE(dragged, nil);
  ASSERT_NE(released, nil);
  EXPECT_EQ(released.type, NSEventTypeLeftMouseUp);
}

TEST_F(MacOSInputEventTargetTest, NativeSiblingButtonReceivesQueuedTap) {
  TapTestAction* action = [[TapTestAction alloc] init];
  AddButton(action);
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown, 280, 95)));
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kUp, 280, 95)));
  ASSERT_TRUE(PumpUntil([&]() { return action.count == 1; }));
}

TEST_F(MacOSInputEventTargetTest, ControllerReleasesInsideNativeButtonTrackingLoop) {
  TapTestAction* action = [[TapTestAction alloc] init];
  AddButton(action);
  auto controller = input::SyntheticGestureController::Create(
      target_, fml::MessageLoop::GetCurrent().GetTaskRunner());
  bool completed = false;
  controller->QueueSyntheticGesture(
      std::make_unique<input::SyntheticTapGesture>(280, 95, 50, input::PointerSourceType::kMouse),
      [&](input::SyntheticGestureResult result) {
        EXPECT_EQ(result, input::SyntheticGestureResult::kDone);
        completed = true;
      });
  ASSERT_TRUE(PumpUntil([&]() { return completed && action.count == 1; }));
}

TEST_F(MacOSInputEventTargetTest, ControllerQueuesRepeatedNativeTaps) {
  TapTestAction* action = [[TapTestAction alloc] init];
  AddButton(action);
  auto controller = input::SyntheticGestureController::Create(
      target_, fml::MessageLoop::GetCurrent().GetTaskRunner());
  int completed = 0;
  for (int i = 0; i < 3; ++i) {
    controller->QueueSyntheticGesture(
        std::make_unique<input::SyntheticTapGesture>(280, 95, 20, input::PointerSourceType::kMouse),
        [&](input::SyntheticGestureResult result) {
          EXPECT_EQ(result, input::SyntheticGestureResult::kDone);
          completed++;
        });
  }
  ASSERT_TRUE(PumpUntil([&]() { return completed == 3 && action.count == 3; }));
}

TEST_F(MacOSInputEventTargetTest, CancelEndsNativeTrackingWithoutClicking) {
  TapTestAction* action = [[TapTestAction alloc] init];
  AddButton(action);
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kDown, 280, 95)));
  ASSERT_TRUE(target_->InjectPointerEvent(Event(input::PointerEventType::kCancel, 280, 95)));
  NSEvent* down = NextMouse();
  ASSERT_NE(down, nil);
  [NSApp sendEvent:down];
  EXPECT_EQ(action.count, 0);
}

}  // namespace
}  // namespace devtool
}  // namespace lynx
