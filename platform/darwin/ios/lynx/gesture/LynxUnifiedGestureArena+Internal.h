// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/** @cond */

#ifndef LYNX_UNIFIED_GESTURE_ARENA_INTERNAL_H_
#define LYNX_UNIFIED_GESTURE_ARENA_INTERNAL_H_

#import "LynxUnifiedGestureArena.h"

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIOwner.h>
#import <QuartzCore/QuartzCore.h>

#import "LynxGestureFlingTrigger.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "core/gesture/gesture_arena.h"
#include "core/gesture/gesture_config.h"

using lynx::tasm::GestureType;
using lynx::tasm::gesture::GestureArena;
using lynx::tasm::gesture::GestureArenaDelegate;
using lynx::tasm::gesture::GestureCallbackType;
using lynx::tasm::gesture::GestureDefinition;
using lynx::tasm::gesture::GestureDirection;
using lynx::tasm::gesture::GestureEvent;
using lynx::tasm::gesture::GestureState;
using lynx::tasm::gesture::GestureStateCommand;
using lynx::tasm::gesture::InputEvent;
using lynx::tasm::gesture::InputType;
using lynx::tasm::gesture::MemberId;
using lynx::tasm::gesture::NormalizeGestureDefinitions;
using lynx::tasm::gesture::ParseGestureCallback;
using CorePoint = lynx::tasm::gesture::Point;
using lynx::tasm::gesture::ScrollResult;
using lynx::tasm::gesture::ScrollState;
using lynx::tasm::gesture::TimerToken;

@class LynxUnifiedGestureArena;

@interface LynxUnifiedGestureArena ()
- (nullable LynxUI *)uiForMember:(MemberId)memberId;
- (void)dispatchGestureEvent:(const GestureEvent &)event;
- (void)scheduleTimer:(TimerToken)token delay:(double)delayMs;
- (void)cancelTimer:(TimerToken)token;
- (BOOL)startFling:(const CorePoint &)velocity;
- (void)stopFling;
- (void)onFling:(LynxGestureFlingTrigger *_Nonnull)flinger;
- (void)onGestureStateChanged:(MemberId)memberId
                    gestureId:(uint32_t)gestureId
                        state:(GestureState)state;
- (void)removeActiveHandlersForMember:(MemberId)memberId;
@end

namespace lynx::tasm::gesture::darwin {

class LynxUnifiedGestureArenaDelegate final : public GestureArenaDelegate {
 public:
  explicit LynxUnifiedGestureArenaDelegate(LynxUnifiedGestureArena *_Nonnull owner)
      : owner_(owner) {}

  bool IsMemberValid(MemberId member_id) const override {
    return [owner_ uiForMember:member_id] != nil;
  }

  CorePoint ConvertPageToMember(MemberId member_id, const CorePoint &page_point) const override {
    LynxUI *ui = [owner_ uiForMember:member_id];
    UIView *rootView = ui.context.rootView;
    if (ui.view == nil || rootView == nil) {
      return page_point;
    }
    CGPoint point = [ui.view convertPoint:CGPointMake(page_point.x, page_point.y)
                                 fromView:rootView];
    return {point.x, point.y};
  }

  ScrollState GetScrollState(MemberId member_id) const override {
    id<LynxGestureArenaMember> member = (id<LynxGestureArenaMember>)[owner_ uiForMember:member_id];
    if (member == nil) {
      return {};
    }
    return {[member getMemberScrollX],
            [member getMemberScrollY],
            [member getGestureBorder:YES],
            [member getGestureBorder:NO]};
  }

  GestureDirection GetScrollDirection(MemberId member_id) const override {
    id<LynxGestureArenaMember> member = (id<LynxGestureArenaMember>)[owner_ uiForMember:member_id];
    if (member == nil) {
      return GestureDirection::kUndetermined;
    }
    switch ([member getScrollContainerDirection]) {
      case DIRECTION_HORIZONTAL:
        return GestureDirection::kHorizontal;
      case DIRECTION_VERTICAL:
        return GestureDirection::kVertical;
      default:
        return GestureDirection::kUndetermined;
    }
  }

  bool CanConsumeGesture(MemberId member_id, GestureDirection direction,
                         const CorePoint &delta) const override {
    (void)direction;
    id<LynxGestureArenaMember> member = (id<LynxGestureArenaMember>)[owner_ uiForMember:member_id];
    return member != nil && [member canConsumeGesture:CGPointMake(delta.x, delta.y)];
  }

  bool ShouldConsumeGesture(MemberId member_id) const override {
    return [owner_ uiForMember:member_id] != nil;
  }

  ScrollResult ScrollBy(MemberId member_id, const CorePoint &delta) override {
    id<LynxGestureArenaMember> member = (id<LynxGestureArenaMember>)[owner_ uiForMember:member_id];
    if (member == nil) {
      return {{}, delta};
    }
    const CorePoint before{[member getMemberScrollX], [member getMemberScrollY]};
    [member onGestureScrollBy:CGPointMake(delta.x, delta.y)];
    const CorePoint consumed{[member getMemberScrollX] - before.x,
                             [member getMemberScrollY] - before.y};
    return {consumed, {delta.x - consumed.x, delta.y - consumed.y}};
  }

  void OnGestureRecognized(MemberId member_id) override {
    LynxUI *ui = [owner_ uiForMember:member_id];
    if (ui != nil) {
      [ui.context onGestureRecognizedByUI:ui];
    }
  }

  void OnGestureStateChanged(MemberId member_id, uint32_t gesture_id, GestureState state) override;

  void DispatchGestureEvent(const GestureEvent &event) override {
    [owner_ dispatchGestureEvent:event];
  }

  void ScheduleTimer(TimerToken token, double delay_ms) override {
    [owner_ scheduleTimer:token delay:delay_ms];
  }

  void CancelTimer(TimerToken token) override { [owner_ cancelTimer:token]; }

  bool StartFling(const CorePoint &velocity) override { return [owner_ startFling:velocity]; }

  void StopFling() override { [owner_ stopFling]; }

 private:
  // The Objective-C arena owns this delegate; keep the back-reference weak.
  __weak LynxUnifiedGestureArena *_Nullable owner_;
};

}  // namespace lynx::tasm::gesture::darwin

using lynx::tasm::gesture::darwin::LynxUnifiedGestureArenaDelegate;

#endif  // LYNX_UNIFIED_GESTURE_ARENA_INTERNAL_H_

// cspell:ignore endcond
/** @endcond */
