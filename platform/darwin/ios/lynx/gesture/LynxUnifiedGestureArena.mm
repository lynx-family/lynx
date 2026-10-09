// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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
- (void)onFling:(LynxGestureFlingTrigger *)flinger;
- (void)onGestureStateChanged:(MemberId)memberId
                    gestureId:(uint32_t)gestureId
                        state:(GestureState)state;
- (void)removeActiveHandlersForMember:(MemberId)memberId;
@end

namespace {

class LynxUnifiedGestureArenaDelegate final : public GestureArenaDelegate {
 public:
  explicit LynxUnifiedGestureArenaDelegate(LynxUnifiedGestureArena *owner) : owner_(owner) {}

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
  __weak LynxUnifiedGestureArena *owner_;
};

std::optional<GestureCallbackType> CallbackType(NSString *name) {
  if ([name isEqualToString:ON_TOUCHES_DOWN]) {
    return GestureCallbackType::kTouchesDown;
  }
  if ([name isEqualToString:ON_TOUCHES_MOVE]) {
    return GestureCallbackType::kTouchesMove;
  }
  if ([name isEqualToString:ON_TOUCHES_UP]) {
    return GestureCallbackType::kTouchesUp;
  }
  if ([name isEqualToString:ON_TOUCHES_CANCEL]) {
    return GestureCallbackType::kTouchesCancel;
  }
  if ([name isEqualToString:ON_BEGIN]) {
    return GestureCallbackType::kBegin;
  }
  if ([name isEqualToString:ON_START]) {
    return GestureCallbackType::kStart;
  }
  if ([name isEqualToString:ON_UPDATE]) {
    return GestureCallbackType::kUpdate;
  }
  if ([name isEqualToString:ON_END]) {
    return GestureCallbackType::kEnd;
  }
  return std::nullopt;
}

void SetConfigNumber(NSDictionary *config, NSString *key, double &target) {
  id value = config[key];
  if ([value isKindOfClass:NSNumber.class]) {
    const double number = [value doubleValue];
    if (std::isfinite(number) && number >= 0) {
      target = number;
    }
  }
}

std::vector<uint32_t> RelationValues(NSDictionary *relations, NSString *key) {
  NSArray *values = relations[key];
  if (![values isKindOfClass:NSArray.class]) {
    return {};
  }
  std::vector<uint32_t> result;
  result.reserve(values.count);
  for (id value in values) {
    if ([value isKindOfClass:NSNumber.class]) {
      result.push_back([value unsignedIntValue]);
    }
  }
  return result;
}

GestureDefinition DefinitionFromDetector(LynxGestureDetectorDarwin *detector) {
  GestureDefinition definition;
  definition.gesture_id = detector.gestureID;
  definition.gesture_type = static_cast<GestureType>(detector.gestureType);
  NSDictionary *config = detector.configMap;
  switch (definition.gesture_type) {
    case GestureType::PAN:
    case GestureType::NATIVE:
      SetConfigNumber(config, @"minDistance", definition.config.min_distance);
      break;
    case GestureType::TAP:
      SetConfigNumber(config, @"maxDistance", definition.config.max_distance);
      SetConfigNumber(config, @"maxDuration", definition.config.max_duration_ms);
      break;
    case GestureType::LONG_PRESS:
      SetConfigNumber(config, @"maxDistance", definition.config.max_distance);
      SetConfigNumber(config, @"minDuration", definition.config.min_duration_ms);
      break;
    case GestureType::DEFAULT:
      SetConfigNumber(config, @"tapSlop", definition.config.tap_slop);
      break;
    case GestureType::FLING:
    case GestureType::ROTATION:
    case GestureType::PINCH:
      break;
  }
  definition.relations.simultaneous = RelationValues(detector.relationMap, @"simultaneous");
  definition.relations.wait_for = RelationValues(detector.relationMap, @"waitFor");
  definition.relations.continue_with = RelationValues(detector.relationMap, @"continueWith");
  for (NSString *name in detector.gestureCallbackNames) {
    auto callback = CallbackType(name);
    if (callback && std::find(definition.callbacks.begin(), definition.callbacks.end(),
                              *callback) == definition.callbacks.end()) {
      definition.callbacks.push_back(*callback);
    }
  }
  return definition;
}

NSString *CallbackName(GestureCallbackType callback) {
  switch (callback) {
    case GestureCallbackType::kTouchesDown:
      return ON_TOUCHES_DOWN;
    case GestureCallbackType::kTouchesMove:
      return ON_TOUCHES_MOVE;
    case GestureCallbackType::kTouchesUp:
      return ON_TOUCHES_UP;
    case GestureCallbackType::kTouchesCancel:
      return ON_TOUCHES_CANCEL;
    case GestureCallbackType::kBegin:
      return ON_BEGIN;
    case GestureCallbackType::kStart:
      return ON_START;
    case GestureCallbackType::kUpdate:
      return ON_UPDATE;
    case GestureCallbackType::kEnd:
      return ON_END;
  }
  return @"";
}

NSString *TouchType(InputType type) {
  switch (type) {
    case InputType::kDown:
      return LynxEventTouchStart;
    case InputType::kMove:
      return LynxEventTouchMove;
    case InputType::kUp:
      return LynxEventTouchEnd;
    case InputType::kCancel:
      return LynxEventTouchCancel;
    case InputType::kFlingFrame:
      return @"unknown";
  }
  return @"unknown";
}

LynxGestureHandlerState DarwinState(GestureState state) {
  switch (state) {
    case GestureState::kInit:
      return LynxGestureHandlerStateInit;
    case GestureState::kBegin:
      return LynxGestureHandlerStateBegin;
    case GestureState::kActive:
      return LynxGestureHandlerStateActive;
    case GestureState::kFail:
      return LynxGestureHandlerStateFail;
    case GestureState::kEnd:
      return LynxGestureHandlerStateEnd;
    case GestureState::kCancel:
      return LynxGestureHandlerStateCancel;
  }
  return LynxGestureHandlerStateInit;
}

InputType CoreInputType(LynxUnifiedGestureInputType type) {
  switch (type) {
    case LynxUnifiedGestureInputTypeDown:
      return InputType::kDown;
    case LynxUnifiedGestureInputTypeMove:
      return InputType::kMove;
    case LynxUnifiedGestureInputTypeUp:
      return InputType::kUp;
    case LynxUnifiedGestureInputTypeCancel:
      return InputType::kCancel;
  }
  return InputType::kCancel;
}

NSString *HandlerKey(MemberId memberId, uint32_t gestureId) {
  return [NSString stringWithFormat:@"%lld:%u", static_cast<long long>(memberId), gestureId];
}

}  // namespace

@implementation LynxUnifiedGestureArena {
  __weak LynxUIOwner *_uiOwner;
  std::shared_ptr<LynxUnifiedGestureArenaDelegate> _delegate;
  std::shared_ptr<GestureArena> _arena;
  NSMutableDictionary<NSNumber *, dispatch_block_t> *_timers;
  NSMutableSet<NSString *> *_activeHandlers;
  LynxGestureFlingTrigger *_flinger;
  InputEvent _lastInput;
  uint64_t _sequenceId;
  BOOL _invalidated;
}

- (instancetype)initWithUIOwner:(LynxUIOwner *)uiOwner {
  self = [super init];
  if (self) {
    _uiOwner = uiOwner;
    _timers = [NSMutableDictionary dictionary];
    _activeHandlers = [NSMutableSet set];
    _flinger = [[LynxGestureFlingTrigger alloc] initWithTarget:self action:@selector(onFling:)];
    _delegate = std::make_shared<LynxUnifiedGestureArenaDelegate>(self);
    _arena = std::make_shared<GestureArena>(*_delegate);
  }
  return self;
}

- (nullable LynxUI *)uiForMember:(MemberId)memberId {
  return [_uiOwner findUIBySign:static_cast<NSInteger>(memberId)];
}

- (void)replaceGestureDetectors:
            (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors
                      forMember:(NSInteger)memberId {
  if (_invalidated || !_arena) {
    return;
  }
  std::vector<GestureDefinition> definitions;
  definitions.reserve(gestureDetectors.count);
  for (LynxGestureDetectorDarwin *detector in gestureDetectors.allValues) {
    definitions.push_back(DefinitionFromDetector(detector));
  }
  definitions = NormalizeGestureDefinitions(std::move(definitions));
  auto delegate = _delegate;
  auto arena = _arena;
  arena->ReplaceMemberGestures(memberId, definitions);
}

- (void)removeMember:(NSInteger)memberId {
  if (_invalidated || !_arena) {
    return;
  }
  auto delegate = _delegate;
  auto arena = _arena;
  arena->RemoveMember(memberId);
  [self removeActiveHandlersForMember:memberId];
}

- (BOOL)containsMember:(NSInteger)memberId {
  return !_invalidated && _arena && _arena->ContainsMember(memberId);
}

- (BOOL)containsGesture:(NSInteger)gestureId memberId:(NSInteger)memberId {
  return !_invalidated && _arena &&
         _arena->ContainsGesture(memberId, static_cast<uint32_t>(gestureId));
}

- (void)setGestureDetectorState:(NSInteger)gestureId
                       memberId:(NSInteger)memberId
                          state:(LynxGestureState)state {
  if (_invalidated || !_arena) {
    return;
  }
  if (![self containsGesture:gestureId memberId:memberId]) {
    return;
  }
  GestureStateCommand command;
  switch (state) {
    case LynxGestureStateActive:
      command = GestureStateCommand::kActive;
      break;
    case LynxGestureStateFail:
      command = GestureStateCommand::kFail;
      break;
    case LynxGestureStateEnd:
      command = GestureStateCommand::kEnd;
      break;
    default:
      return;
  }
  auto delegate = _delegate;
  auto arena = _arena;
  arena->SetGestureState(memberId, static_cast<uint32_t>(gestureId), command);
}

- (void)handleTouch:(nullable UITouch *)touch
             action:(LynxUnifiedGestureInputType)action
             target:(nullable id<LynxEventTarget>)target
          pointerId:(NSInteger)pointerId
           velocity:(CGPoint)velocity {
  if (_invalidated || !_arena || touch == nil || target == nil) {
    return;
  }
  if (action == LynxUnifiedGestureInputTypeDown) {
    ++_sequenceId;
  }
  CGPoint pagePoint = [touch locationInView:_uiOwner.uiContext.rootView];
  CGPoint clientPoint = [touch locationInView:nil];
  InputEvent input;
  input.type = CoreInputType(action);
  input.sequence_id = _sequenceId;
  input.pointer_id = pointerId;
  input.monotonic_time_ms = touch.timestamp * 1000.0;
  input.epoch_time_ms = static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000.0);
  input.screen = {clientPoint.x, clientPoint.y};
  input.page = {pagePoint.x, pagePoint.y};
  input.client = input.screen;
  input.velocity = {velocity.x, velocity.y};
  _lastInput = input;

  std::vector<MemberId> responseChain;
  id<LynxEventTarget> candidate = target;
  while (candidate != nil) {
    MemberId memberId = candidate.signature;
    if (_arena->ContainsMember(memberId)) {
      responseChain.push_back(memberId);
    }
    candidate = candidate.parentTarget;
  }
  auto delegate = _delegate;
  auto arena = _arena;
  arena->HandleInput(input, responseChain);
}

- (void)dispatchGestureEvent:(const GestureEvent &)event {
  LynxUI *ui = [self uiForMember:event.member_id];
  if (ui == nil) {
    return;
  }
  GestureType type = event.gesture_type;
  NSMutableDictionary *params = [NSMutableDictionary dictionary];
  params[@"timestamp"] = @(event.timestamp_epoch_ms);
  params[@"type"] = TouchType(event.source);
  params[@"x"] = @(event.local.x);
  params[@"y"] = @(event.local.y);
  params[@"pageX"] = @(event.page.x);
  params[@"pageY"] = @(event.page.y);
  params[@"clientX"] = @(event.client.x);
  params[@"clientY"] = @(event.client.y);
  if (type == GestureType::PAN || type == GestureType::NATIVE || type == GestureType::DEFAULT ||
      type == GestureType::FLING) {
    params[@"scrollX"] = @(event.scroll.x);
    params[@"scrollY"] = @(event.scroll.y);
    params[@"isAtStart"] = @(event.scroll.is_at_start);
    params[@"isAtEnd"] = @(event.scroll.is_at_end);
  }
  if (type == GestureType::DEFAULT || type == GestureType::FLING) {
    params[@"deltaX"] = @(event.delta.x);
    params[@"deltaY"] = @(event.delta.y);
  }
  LynxCustomEvent *customEvent = [[LynxCustomEvent alloc] initWithName:CallbackName(event.callback)
                                                            targetSign:event.member_id
                                                                params:params];
  [ui.context.eventEmitter dispatchGestureEvent:event.gesture_id event:customEvent];
}

- (void)onGestureStateChanged:(MemberId)memberId
                    gestureId:(uint32_t)gestureId
                        state:(GestureState)state {
  NSString *key = HandlerKey(memberId, gestureId);
  if (state == GestureState::kActive) {
    [_activeHandlers addObject:key];
  } else {
    [_activeHandlers removeObject:key];
  }
  id<LynxGestureArenaMember> member = (id<LynxGestureArenaMember>)[self uiForMember:memberId];
  [member onPlatformGestureStatusChanged:DarwinState(state)];
}

- (void)removeActiveHandlersForMember:(MemberId)memberId {
  NSString *prefix = [NSString stringWithFormat:@"%lld:", static_cast<long long>(memberId)];
  NSPredicate *predicate = [NSPredicate predicateWithBlock:^BOOL(NSString *key, NSDictionary *_) {
    return [key hasPrefix:prefix];
  }];
  [_activeHandlers minusSet:[_activeHandlers filteredSetUsingPredicate:predicate]];
}

- (BOOL)hasActiveGesture {
  return _activeHandlers.count > 0;
}

- (void)scheduleTimer:(TimerToken)token delay:(double)delayMs {
  [self cancelTimer:token];
  NSNumber *key = @(token);
  __weak typeof(self) weakSelf = self;
  dispatch_block_t block = dispatch_block_create(static_cast<dispatch_block_flags_t>(0), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || strongSelf->_invalidated || !strongSelf->_arena) {
      return;
    }
    [strongSelf->_timers removeObjectForKey:key];
    auto delegate = strongSelf->_delegate;
    auto arena = strongSelf->_arena;
    arena->HandleTimer(token, CACurrentMediaTime() * 1000.0,
                       static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000.0));
  });
  _timers[key] = block;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(delayMs * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), block);
}

- (void)cancelTimer:(TimerToken)token {
  NSNumber *key = @(token);
  dispatch_block_t block = _timers[key];
  if (block != nil) {
    dispatch_block_cancel(block);
    [_timers removeObjectForKey:key];
  }
}

- (BOOL)startFling:(const CorePoint &)velocity {
  return [_flinger startWithVelocity:CGPointMake(velocity.x, velocity.y)];
}

- (void)stopFling {
  [_flinger reset];
}

- (void)onFling:(LynxGestureFlingTrigger *)flinger {
  if (_invalidated || !_arena || flinger.state == LynxGestureFlingTriggerStateStart) {
    return;
  }
  InputEvent input = _lastInput;
  input.type = InputType::kFlingFrame;
  input.monotonic_time_ms = CACurrentMediaTime() * 1000.0;
  input.epoch_time_ms = static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000.0);
  input.delta = {flinger.lastDistance.x - flinger.distance.x,
                 flinger.lastDistance.y - flinger.distance.y};
  input.fling_finished = flinger.state == LynxGestureFlingTriggerStateEnd;
  auto delegate = _delegate;
  auto arena = _arena;
  arena->HandleInput(input);
}

- (void)invalidate {
  if (_invalidated) {
    return;
  }
  _invalidated = YES;
  auto delegate = std::move(_delegate);
  auto arena = std::move(_arena);
  if (arena) {
    arena->Reset();
  }
  for (dispatch_block_t block in _timers.allValues) {
    dispatch_block_cancel(block);
  }
  [_timers removeAllObjects];
  [_flinger reset];
  [_activeHandlers removeAllObjects];
}

- (void)dealloc {
  [self invalidate];
}

@end

void LynxUnifiedGestureArenaDelegate::OnGestureStateChanged(MemberId member_id, uint32_t gesture_id,
                                                            GestureState state) {
  [owner_ onGestureStateChanged:member_id gestureId:gesture_id state:state];
}
