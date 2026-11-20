// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUnifiedGestureArena+Internal.h"

namespace {

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
    auto callback = ParseGestureCallback(name.UTF8String ?: "");
    if (callback && std::find(definition.callbacks.begin(), definition.callbacks.end(),
                              *callback) == definition.callbacks.end()) {
      definition.callbacks.push_back(*callback);
    }
  }
  return definition;
}

std::vector<GestureDefinition> DefinitionsFromDetectors(
    NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *detectors) {
  std::vector<GestureDefinition> definitions;
  definitions.reserve(detectors.count);
  for (LynxGestureDetectorDarwin *detector in detectors.allValues) {
    definitions.push_back(DefinitionFromDetector(detector));
  }
  return NormalizeGestureDefinitions(std::move(definitions));
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
  auto definitions = DefinitionsFromDetectors(gestureDetectors);
  // Retain both C++ objects across callbacks that can invalidate the wrapper.
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

- (LynxUnifiedGestureRegistration)registrationForDetectors:
    (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors {
  if (_invalidated || !_arena) {
    return 0;
  }
  // Dispatch membership can be invalidated while a replacement waits for a callback to return.
  const auto definitions = DefinitionsFromDetectors(gestureDetectors);
  if (definitions.empty()) {
    return 0;
  }
  const bool hasNative =
      std::any_of(definitions.begin(), definitions.end(), [](const GestureDefinition &definition) {
        return definition.gesture_type == GestureType::NATIVE;
      });
  return LynxUnifiedGestureRegistrationHasDetector |
         (hasNative ? LynxUnifiedGestureRegistrationHasNativeDetector : 0);
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
  // UIKit points already match the core's logical coordinate space.
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
  params[@"type"] = [NSString stringWithUTF8String:GestureInputTypeName(event.source)];
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
  LynxCustomEvent *customEvent = [[LynxCustomEvent alloc]
      initWithName:[NSString stringWithUTF8String:GestureCallbackName(event.callback)]
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
  // Detach first so reset callbacks cannot reenter the old arena.
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
