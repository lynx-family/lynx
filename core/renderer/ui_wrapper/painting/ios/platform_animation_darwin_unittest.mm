// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxAnimationDelegate.h>
#import <Lynx/LynxConverter+Transform.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxPlatformAnimation.h>
#import <Lynx/LynxTransitionAnimationManager.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIContext.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#include "core/renderer/ui_wrapper/painting/ios/platform_animation_darwin.h"

namespace {
using namespace lynx;
using EventState = gfx::AnimationPlaybackState::EventState;
NSString* const kAnimationName = @"TestAnimation";

gfx::PlatformAnimationCommand OpacityCommand(uint32_t generation = 1, long duration = 10000,
                                             int iterations = 1, float from = 0.2f,
                                             float to = 0.8f) {
  gfx::PlatformAnimationCommand command;
  command.animation_id = 1;
  command.generation = generation;
  command.name = kAnimationName.UTF8String;
  command.type = generation == 1 ? gfx::PlatformAnimationCommandType::kHandoff
                                 : gfx::PlatformAnimationCommandType::kUpdate;
  command.animation_data.duration = duration;
  command.animation_data.iteration_count = iterations;
  command.animation_data.fill_mode = gfx::AnimationFillModeType::kForwards;
  gfx::PlatformAnimationProperty property;
  property.property = gfx::AnimationPropertyType::kOpacity;
  for (int i = 0; i < 2; ++i) {
    auto keyframe = gfx::FloatKeyframe::Create(lynx::fml::TimeDelta::FromSeconds(i));
    keyframe->SetValue(i == 0 ? from : to);
    property.keyframes.push_back(std::move(keyframe));
  }
  command.properties.push_back(std::move(property));
  return command;
}

gfx::PlatformAnimationCommand TransformCommand(const gfx::TransformOperations& from,
                                               const gfx::TransformOperations& to) {
  auto command = OpacityCommand();
  auto& property = command.properties.front();
  property.property = gfx::AnimationPropertyType::kTransform;
  property.keyframes.clear();
  for (int i = 0; i < 2; ++i) {
    auto keyframe = gfx::TransformKeyframe::Create(lynx::fml::TimeDelta::FromSeconds(i));
    keyframe->SetResolvedValue(i == 0 ? from : to);
    property.keyframes.push_back(std::move(keyframe));
  }
  return command;
}

void Apply(LynxUI* ui, gfx::PlatformAnimationCommand command) {
  auto commands = lynx::fml::MakeRefCounted<gfx::PlatformAnimationCommandBatch>();
  commands->push_back(std::move(command));
  tasm::ApplyPlatformAnimationCommands(ui, commands);
}

lynx::fml::RefPtr<gfx::PlatformTransitionSnapshot> DetachTransition(LynxUI* ui, uint32_t generation,
                                                                    bool capture_timeline = false) {
  auto snapshot = lynx::fml::MakeRefCounted<gfx::PlatformTransitionSnapshot>(capture_timeline);
  gfx::PlatformAnimationCommand command;
  command.kind = gfx::AnimationKind::kTransition;
  command.type = gfx::PlatformAnimationCommandType::kDetach;
  command.animation_id = 1;
  command.generation = generation;
  command.transition_snapshot = snapshot;
  Apply(ui, std::move(command));
  return snapshot;
}

id HasElapsedTime(NSTimeInterval elapsedTime) {
  return [OCMArg checkWithBlock:^BOOL(NSDictionary* params) {
    return [params[@"elapsedTime"] isEqual:@(elapsedTime)];
  }];
}
}  // namespace

// Keep CAAnimation copies, but deliver callbacks explicitly so Core Animation
// cannot finish animations while the test drains the main queue.
@interface ManuallyDrivenAnimationLayer : CALayer
@property(nonatomic, strong) NSMutableDictionary<NSString*, CAAnimation*>* storedAnimations;
@property(nonatomic, strong) ManuallyDrivenAnimationLayer* displayedLayer;
@end
@implementation ManuallyDrivenAnimationLayer
- (instancetype)presentationLayer {
  return _displayedLayer;
}
- (void)addAnimation:(CAAnimation*)animation forKey:(NSString*)key {
  if (_storedAnimations == nil) {
    _storedAnimations = [NSMutableDictionary new];
  }
  _storedAnimations[key] = [animation copy];
}
- (CAAnimation*)animationForKey:(NSString*)key {
  return [_storedAnimations[key] copy];
}
- (NSArray<NSString*>*)animationKeys {
  return _storedAnimations.allKeys;
}
- (void)removeAnimationForKey:(NSString*)key {
  [_storedAnimations removeObjectForKey:key];
}
- (void)removeAllAnimations {
  [_storedAnimations removeAllObjects];
}
@end

@interface ManuallyDrivenAnimationView : UIView
@end
@implementation ManuallyDrivenAnimationView
+ (Class)layerClass {
  return [ManuallyDrivenAnimationLayer class];
}
@end

@interface PlatformAnimationDarwinTest : XCTestCase
@property(nonatomic, strong) LynxUI* ui;
@property(nonatomic, strong) id events;
@end

@implementation PlatformAnimationDarwinTest
- (void)setUp {
  [super setUp];
  self.ui = [[LynxUI alloc]
      initWithView:[[ManuallyDrivenAnimationView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)]];
  self.ui.updatedFrame = self.ui.view.bounds;
  self.ui.view.layer.opacity = 0.4;
  self.events = OCMClassMock([LynxAnimationDelegate class]);
}

- (void)tearDown {
  [self.ui.animationManager endAllAnimation];
  [self.events stopMocking];
  self.ui = nil;
  self.events = nil;
  [super tearDown];
}

- (CAKeyframeAnimation*)animationForKeyPath:(NSString*)keyPath {
  return (CAKeyframeAnimation*)[self.ui.view.layer
      animationForKey:[keyPath stringByAppendingString:kAnimationName]];
}

- (void)drainMainQueue {
  XCTestExpectation* expectation = [self expectationWithDescription:@"animation completion"];
  dispatch_async(dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:1];
}

- (void)testPausedHandoffUsesLogicalTimeAndDetachPreservesStyle {
  auto playback = lynx::fml::MakeRefCounted<gfx::AnimationPlaybackState>(
      lynx::fml::TimeDelta::FromSeconds(3), true, EventState::kStarted);
  auto command = OpacityCommand(1, 10000, -1);
  command.playback = playback;
  command.executor = playback->ChangeExecutor();
  command.animation_data.delay = 1000;
  command.animation_data.play_state = gfx::AnimationPlayStateType::kPaused;
  command.animation_data.direction = gfx::AnimationDirectionType::kReverse;
  OCMReject([self.events sendAnimationEvent:self.ui
                                  eventName:[OCMArg any]
                                eventParams:[OCMArg any]]);
  Apply(self.ui, std::move(command));
  CAKeyframeAnimation* native = [self animationForKeyPath:@"opacity"];
  XCTAssertNotNil(native);
  XCTAssertEqual(native.repeatCount, 1e9f);
  XCTAssertEqual(native.speed, 0);
  XCTAssertEqualWithAccuracy(native.timeOffset, 2, 0.001);
  XCTAssertEqualWithAccuracy([native.values.firstObject doubleValue], 0.8, 0.001);
  [native.delegate animationDidStop:native finished:NO];
  XCTAssertNotNil([self animationForKeyPath:@"opacity"]);

  playback->ChangeExecutor();
  // A stale callback can arrive before the detach command reaches the UI.
  [native.delegate animationDidStop:native finished:YES];
  XCTAssertNotNil([self animationForKeyPath:@"opacity"]);
  self.ui.view.layer.opacity = 0.65;
  auto detach = OpacityCommand();
  detach.type = gfx::PlatformAnimationCommandType::kDetach;
  Apply(self.ui, std::move(detach));
  XCTAssertNil([self animationForKeyPath:@"opacity"]);
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.65, 0.001);
  XCTAssertTrue(playback->event_state() == EventState::kStarted);
}

- (void)testCompletedHandoffRestoresFillEvenWhenPaused {
  OCMReject([self.events sendAnimationEvent:self.ui
                                  eventName:[OCMArg any]
                                eventParams:[OCMArg any]]);
  for (long duration : {10000L, 0L}) {
    auto command = OpacityCommand(1, duration);
    command.animation_id = duration + 1;
    command.playback = lynx::fml::MakeRefCounted<gfx::AnimationPlaybackState>(
        lynx::fml::TimeDelta::FromSeconds(12), true, EventState::kFinished);
    command.executor = command.playback->ChangeExecutor();
    command.animation_data.play_state = gfx::AnimationPlayStateType::kPaused;
    command.animation_data.direction = gfx::AnimationDirectionType::kReverse;
    Apply(self.ui, std::move(command));
    XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.2, 0.001);
    XCTAssertEqual(self.ui.view.layer.animationKeys.count, 0u);
    XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
  }
}

- (void)testStaleCompletionIsIgnoredAndRemovalClaimsCancel {
  auto playback = lynx::fml::MakeRefCounted<gfx::AnimationPlaybackState>(
      lynx::fml::TimeDelta::FromSeconds(3), true, EventState::kStarted);
  const auto executor = playback->ChangeExecutor();
  auto command = OpacityCommand();
  command.playback = playback;
  command.executor = executor;
  Apply(self.ui, std::move(command));
  CAAnimation* previous = [self animationForKeyPath:@"opacity"];
  auto update = OpacityCommand(2, 20000);
  update.reuse_keyframes = true;
  update.properties.clear();
  update.playback = playback;
  update.executor = executor;
  update.animation_data.delay = 1000;
  Apply(self.ui, std::move(update));
  [previous.delegate animationDidStop:previous finished:YES];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  XCTAssertEqual([self animationForKeyPath:@"opacity"].duration, 20);

  self.ui.animationManager.autoResumeAnimation = NO;
  OCMExpect([self.events sendAnimationEvent:self.ui
                                  eventName:@"animationcancel"
                                eventParams:HasElapsedTime(2.0)]);
  auto cancel = OpacityCommand(2, 20000);
  cancel.type = gfx::PlatformAnimationCommandType::kCancel;
  cancel.playback = playback;
  cancel.executor = executor;
  Apply(self.ui, std::move(cancel));
  OCMVerifyAll(self.events);
  XCTAssertTrue(playback->event_state() == EventState::kCanceled);
  XCTAssertNil([self animationForKeyPath:@"opacity"]);
}

- (void)testLegacyTransitionForcedStopReportsActiveTime {
  auto* manager = [[LynxTransitionAnimationManager alloc] initWithLynxUI:self.ui];
  CALayer* layer = self.ui.view.layer;
  layer.speed = 0;
  layer.timeOffset = 10;
  // Configure a two-second opacity transition with a 250ms delay.
  [manager assembleTransitions:@[ @[ @(OPACITY), @2000, @0, @0, @0, @0, @0, @0, @250 ] ]];
  [manager performTransitionAnimationsWithOpacity:0.8
                                         callback:^(BOOL finished) {
                                           XCTAssertFalse(finished);
                                         }];
  [manager applyTransitionAnimation];
  CAAnimation* animation = [layer animationForKey:@"transition-opacity"];
  XCTAssertNotNil(animation);
  layer.timeOffset = animation.beginTime + 0.75;
  OCMExpect([self.events sendAnimationEvent:self.ui
                                  eventName:@"transitionend"
                                eventParams:HasElapsedTime(0.75)]);
  [manager removeTransitionAnimation:OPACITY];
  OCMVerifyAll(self.events);
}

- (void)testInstantEffectsApplyFillAndSendEvents {
  using Fill = gfx::AnimationFillModeType;
  const struct {
    long duration;
    int iterations;
    Fill fill;
    float expected;
  } cases[] = {
      {0, 1, Fill::kForwards, 0.8f}, {0, 1, Fill::kNone, 0.4f}, {10000, 0, Fill::kForwards, 0.2f}};
  uint64_t animation_id = 0;
  for (const auto& test : cases) {
    self.ui.view.layer.opacity = 0.4;
    auto command = OpacityCommand(1, test.duration, test.iterations);
    command.animation_id = ++animation_id;
    command.animation_data.fill_mode = test.fill;
    OCMExpect([self.events sendAnimationEvent:self.ui
                                    eventName:@"animationstart"
                                  eventParams:[OCMArg any]]);
    OCMExpect([self.events sendAnimationEvent:self.ui
                                    eventName:@"animationend"
                                  eventParams:[OCMArg any]]);
    Apply(self.ui, std::move(command));
    [self drainMainQueue];
    XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, test.expected, 0.001);
    XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
    XCTAssertEqual(self.ui.view.layer.animationKeys.count, 0u);
    OCMVerifyAll(self.events);
  }
}

- (void)testRoutedStartWaitsForDelay {
  for (long duration : {0L, 10000L}) {
    auto playback = lynx::fml::MakeRefCounted<gfx::AnimationPlaybackState>(
        lynx::fml::TimeDelta::Zero(), true, EventState::kNotStarted);
    const auto executor = playback->ChangeExecutor();
    auto apply = [&](uint32_t generation) {
      auto command = OpacityCommand(generation, duration);
      command.animation_data.delay = 5000;
      command.playback = playback;
      command.executor = executor;
      Apply(self.ui, std::move(command));
    };
    apply(1);
    [self drainMainQueue];
    XCTAssertTrue(playback->event_state() == EventState::kNotStarted);
    playback->SetCurrentTime(lynx::fml::TimeDelta::FromSeconds(5));
    apply(2);
    if (duration) {
      CAAnimation* current = [self animationForKeyPath:@"opacity"];
      [current.delegate animationDidStart:current];
    }
    XCTAssertTrue(playback->event_state() ==
                  (duration ? EventState::kStarted : EventState::kFinished));
    [self.ui.animationManager endAllAnimation];
  }
}

- (void)testResizeRebuildsPercentageTransforms {
  gfx::TransformOperations operations;
  operations.AppendTranslate({25.f, gfx::LengthUnit::kPercent}, {0.f, gfx::LengthUnit::kNumber},
                             {0.f, gfx::LengthUnit::kNumber});
  Apply(self.ui, TransformCommand({}, operations));
  CATransform3D transform =
      [[self animationForKeyPath:@"transform"].values.lastObject CATransform3DValue];
  XCTAssertEqualWithAccuracy(transform.m41, 25, 0.001);

  self.ui.updatedFrame = CGRectMake(0, 0, 200, 100);
  auto update = TransformCommand({}, operations);
  update.generation = 2;
  update.type = gfx::PlatformAnimationCommandType::kUpdate;
  update.reuse_keyframes = true;
  update.properties.clear();
  Apply(self.ui, std::move(update));
  transform = [[self animationForKeyPath:@"transform"].values.lastObject CATransform3DValue];
  XCTAssertEqualWithAccuracy(transform.m41, 50, 0.001);

  self.ui.updatedFrame = CGRectMake(0, 0, 300, 100);
  [self.ui.animationManager notifyAnimationUpdated];
  transform = [[self animationForKeyPath:@"transform"].values.lastObject CATransform3DValue];
  XCTAssertEqualWithAccuracy(transform.m41, 75, 0.001);
}

- (void)testTypedTransformConstructionMatchesLegacyConversion {
  gfx::TransformOperations operations;
  operations.AppendTranslate({25.f, gfx::LengthUnit::kPercent}, {12.f, gfx::LengthUnit::kNumber},
                             {3.f, gfx::LengthUnit::kNumber});
  operations.AppendScale(2.f, 3.f);
  operations.AppendSkew(10.f, 20.f);
  const std::array<float, 16> matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 7, 8, 9, 1};
  operations.AppendMatrix(gfx::TransformOperation::kMatrix3d, matrix);
  NSArray* legacy = @[
    @[
      @(LynxTransformTypeTranslate3d), @0.25, @(LynxPlatformLengthUnitPercentage), @12,
      @(LynxPlatformLengthUnitNumber), @3, @(LynxPlatformLengthUnitNumber)
    ],
    @[ @(LynxTransformTypeScale), @2, @0, @3, @0, @0, @0 ],
    @[ @(LynxTransformTypeSkew), @10, @0, @20, @0, @0, @0 ],
    @[
      @(LynxTransformTypeMatrix3d), @1, @0, @0, @0, @0, @1, @0, @0, @0, @0, @1, @0, @7, @8, @9, @1
    ]
  ];
  CATransform3D expected = [LynxConverter toCATransform3D:[LynxTransformRaw toTransformRaw:legacy]
                                                       ui:self.ui];
  auto from = operations;
  operations.AppendRotate(gfx::TransformOperation::kRotateZ, -360.f);
  auto command = TransformCommand(from, operations);
  auto opacity = OpacityCommand();
  command.properties.push_back(std::move(opacity.properties.front()));
  Apply(self.ui, std::move(command));
  CAKeyframeAnimation* animation = [self animationForKeyPath:@"transform"];
  XCTAssertNotNil(animation);
  XCTAssertTrue(
      CATransform3DEqualToTransform([animation.values.firstObject CATransform3DValue], expected));
  XCTAssertEqualWithAccuracy(
      [[self animationForKeyPath:@"transform.rotation.z"].values.lastObject doubleValue], -2 * M_PI,
      0.001);
  XCTAssertEqualWithAccuracy([[self animationForKeyPath:@"opacity"].values.lastObject doubleValue],
                             0.8, 0.001);
}

- (void)testTransformTransitionRetargetsAndPreservesModelStyle {
  NSString* const key = @"transition-transform";
  auto commandForTarget = [](float from, float to, uint32_t generation) {
    gfx::TransformOperations start, end;
    start.AppendTranslate({from, gfx::LengthUnit::kNumber}, {}, {});
    end.AppendTranslate({to, gfx::LengthUnit::kNumber}, {}, {});
    auto command = TransformCommand(start, end);
    command.kind = gfx::AnimationKind::kTransition;
    command.type = generation == 1 ? gfx::PlatformAnimationCommandType::kHandoff
                                   : gfx::PlatformAnimationCommandType::kUpdate;
    command.generation = generation;
    return command;
  };
  auto* layer = (ManuallyDrivenAnimationLayer*)self.ui.view.layer;
  Apply(self.ui, commandForTarget(20, 100, 1));
  CAAnimation* previous = [layer animationForKey:key];
  XCTAssertNotNil(previous);
  id<CAAnimationDelegate> previousDelegate = previous.delegate;

  LynxTransformRaw* model = [LynxTransformRaw new];
  model.type = LynxTransformTypeMatrix3d;
  model.transformMatrix = CATransform3DMakeTranslation(200, 0, 0);
  self.ui.transformRaw = @[ model ];
  [self.ui setValue:@YES forKey:@"didTransformChanged"];
  [self.ui onNodeReadyForUIOwner];
  XCTAssertNotNil([layer animationForKey:key]);

  Apply(self.ui, commandForTarget(100, 150, 2));
  CAAnimationGroup* pending = (CAAnimationGroup*)[layer animationForKey:key];
  CABasicAnimation* pendingTransform = (CABasicAnimation*)pending.animations.firstObject;
  XCTAssertEqualWithAccuracy([pendingTransform.fromValue CATransform3DValue].m41, 20, 1e-5);

  layer.displayedLayer = [ManuallyDrivenAnimationLayer layer];
  layer.displayedLayer.transform = CATransform3DMakeTranslation(50, 0, 0);
  Apply(self.ui, commandForTarget(150, 200, 3));
  CAAnimationGroup* current = (CAAnimationGroup*)[layer animationForKey:key];
  CABasicAnimation* transform = (CABasicAnimation*)current.animations.firstObject;
  XCTAssertEqualWithAccuracy([transform.fromValue CATransform3DValue].m41, 50, 1e-5);
  XCTAssertEqualWithAccuracy([transform.toValue CATransform3DValue].m41, 200, 1e-5);
  XCTAssertNotNil(previousDelegate);
  [previousDelegate animationDidStop:previous finished:YES];
  XCTAssertNotNil([layer animationForKey:key]);

  [current.delegate animationDidStop:current finished:YES];
  XCTAssertNil([layer animationForKey:key]);
}

- (void)testOpacityKeyframesKeepTransitionAndItsEvents {
  [self.events stopMocking];
  self.events = nil;
  id emitter = OCMStrictClassMock([LynxEventEmitter class]);
  id context = OCMClassMock([LynxUIContext class]);
  OCMStub([context eventEmitter]).andReturn(emitter);
  self.ui.context = context;
  [self.ui setRawEvents:[NSSet setWithObjects:@"transitionstart(bindEvent)",
                                              @"transitionend(bindEvent)", nil]
      andLepusRawEvents:nil];
  id (^eventMatching)(NSString*, double) = ^id(NSString* name, double elapsedTime) {
    return [OCMArg checkWithBlock:^BOOL(LynxCustomEvent* event) {
      return [event.eventName isEqualToString:name] && [event.params isEqualToDictionary:@{
               @"new_animator" : @YES,
               @"animation_type" : @"transition-animation",
               @"animation_name" : @"opacity",
               @"elapsedTime" : @(elapsedTime)
             }];
    }];
  };

  auto transition = OpacityCommand(1, 2000);
  transition.kind = gfx::AnimationKind::kTransition;
  transition.name = "opacity";
  Apply(self.ui, std::move(transition));
  CALayer* layer = self.ui.view.layer;
  NSString* const key = @"transition-opacity";
  CAAnimation* initial = [layer animationForKey:key];
  XCTAssertNotNil(initial);
  OCMExpect([emitter dispatchCustomEvent:eventMatching(@"transitionstart", 0)]);
  [initial.delegate animationDidStart:initial];
  [initial.delegate animationDidStart:initial];

  uint32_t generation = 0;
  for (int delay : {0, 500}) {
    auto keyframe = OpacityCommand(++generation, 100, 1, 0.8f, 0.8f);
    keyframe.animation_data.delay = delay;
    keyframe.animation_data.fill_mode = gfx::AnimationFillModeType::kNone;
    auto playback = lynx::fml::MakeRefCounted<gfx::AnimationPlaybackState>(
        lynx::fml::TimeDelta::Zero(), false, EventState::kNotStarted);
    keyframe.playback = playback;
    keyframe.executor = playback->ChangeExecutor();
    Apply(self.ui, std::move(keyframe));

    CAKeyframeAnimation* native = [self animationForKeyPath:@"opacity"];
    XCTAssertNotNil(native);
    playback->SetCurrentTime(lynx::fml::TimeDelta::FromMilliseconds(delay));
    [native.delegate animationDidStart:native];
    XCTAssertNotNil([layer animationForKey:key]);
    [native.delegate animationDidStop:native finished:YES];
    XCTAssertNil([self animationForKeyPath:@"opacity"]);

    CAAnimation* remaining = [layer animationForKey:key];
    XCTAssertNotNil(remaining);
    XCTAssertEqual(remaining.beginTime, initial.beginTime);
    XCTAssertEqual(remaining.duration, initial.duration);
  }

  OCMExpect([emitter dispatchCustomEvent:eventMatching(@"transitionend", 2)]);
  [initial.delegate animationDidStop:initial finished:YES];
  [initial.delegate animationDidStop:initial finished:YES];
  OCMVerify(times(1), [emitter dispatchCustomEvent:eventMatching(@"transitionstart", 0)]);
  OCMVerify(times(1), [emitter dispatchCustomEvent:eventMatching(@"transitionend", 2)]);
  [context stopMocking];
}

- (void)testOpacityTransitionReportsCancelToLateListener {
  [self.events stopMocking];
  self.events = nil;
  id emitter = OCMStrictClassMock([LynxEventEmitter class]);
  id context = OCMClassMock([LynxUIContext class]);
  OCMStub([context eventEmitter]).andReturn(emitter);
  self.ui.context = context;

  auto transition = OpacityCommand(1, 2000);
  transition.kind = gfx::AnimationKind::kTransition;
  transition.name = "opacity";
  transition.animation_data.delay = 250;
  CALayer* layer = self.ui.view.layer;
  layer.speed = 0;
  layer.timeOffset = 10;
  Apply(self.ui, std::move(transition));
  NSString* const key = @"transition-opacity";
  CAAnimation* native = [layer animationForKey:key];
  XCTAssertNotNil(native);
  layer.timeOffset = native.beginTime + 0.5;
  [native.delegate animationDidStart:native];

  [self.ui setRawEvents:[NSSet setWithObject:@"transitioncancel(bindEvent)"] andLepusRawEvents:nil];
  id event = [OCMArg checkWithBlock:^BOOL(LynxCustomEvent* value) {
    return [value.eventName isEqualToString:@"transitioncancel"] &&
           [value.params[@"animation_name"] isEqual:@"opacity"] &&
           [value.params[@"elapsedTime"] isEqual:@0.5];
  }];
  OCMExpect([emitter dispatchCustomEvent:event]);
  for (int i = 0; i < 2; ++i) {
    gfx::PlatformAnimationCommand cancel;
    cancel.type = gfx::PlatformAnimationCommandType::kCancel;
    cancel.kind = gfx::AnimationKind::kTransition;
    cancel.animation_id = 1;
    cancel.generation = 1;
    Apply(self.ui, std::move(cancel));
  }
  [native.delegate animationDidStop:native finished:NO];
  OCMVerify(times(1), [emitter dispatchCustomEvent:event]);
  XCTAssertNil([layer animationForKey:key]);
  [context stopMocking];
}

- (void)testOpacityTransitionRetargetsBeforeFirstFrameAndCancels {
  auto command = OpacityCommand();
  command.kind = gfx::AnimationKind::kTransition;
  Apply(self.ui, std::move(command));
  auto* layer = (ManuallyDrivenAnimationLayer*)self.ui.view.layer;
  NSString* const key = @"transition-opacity";
  CAAnimation* previous = [layer animationForKey:key];
  XCTAssertNotNil(previous);
  OCMReject([self.events sendAnimationEvent:self.ui
                                  eventName:@"transitionend"
                                eventParams:[OCMArg any]]);

  auto update = OpacityCommand(2, 10000, 1, 0.8f, 0.4f);
  update.kind = gfx::AnimationKind::kTransition;
  Apply(self.ui, std::move(update));
  CABasicAnimation* animation = (CABasicAnimation*)[layer animationForKey:key];
  XCTAssertEqualWithAccuracy([animation.fromValue doubleValue], 0.2, 1e-5);
  XCTAssertEqualWithAccuracy([animation.toValue doubleValue], 0.4, 1e-5);

  [previous.delegate animationDidStop:previous finished:YES];
  auto snapshot = DetachTransition(self.ui, 2);
  [animation.delegate animationDidStop:animation finished:YES];
  const gfx::PlatformTransitionSnapshot::Value* captured = nullptr;
  XCTAssertTrue(snapshot->Read(captured));
  XCTAssertEqualWithAccuracy(captured ? *captured : -1, 0.2, 1e-5);
  CABasicAnimation* hold = (CABasicAnimation*)[layer animationForKey:key];
  XCTAssertEqualObjects(hold.keyPath, @"opacity");
  XCTAssertEqualWithAccuracy([hold.fromValue doubleValue], 0.2, 1e-5);
  layer.opacity = 0.4;
  gfx::PlatformAnimationCommand release;
  release.type = gfx::PlatformAnimationCommandType::kDetach;
  release.kind = gfx::AnimationKind::kTransition;
  release.animation_id = 1;
  release.generation = 2;
  Apply(self.ui, std::move(release));
  XCTAssertNil([layer animationForKey:key]);
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.4, 1e-5);

  auto next = OpacityCommand(3, 2000, 1, 0.4f, 1.f);
  next.kind = gfx::AnimationKind::kTransition;
  next.animation_data.delay = 250;
  layer.speed = 0;
  layer.timeOffset = 10;
  Apply(self.ui, std::move(next));
  CAAnimation* running = [layer animationForKey:key];
  XCTAssertNotNil(running);
  layer.timeOffset = running.beginTime + 0.5;
  [running.delegate animationDidStart:running];
  const auto expected = lynx::fml::TimeDelta::FromMilliseconds(750);
  const auto earliest_start = lynx::fml::TimePoint::Now() - expected;
  OCMReject([self.events sendAnimationEvent:self.ui
                                  eventName:@"transitioncancel"
                                eventParams:[OCMArg any]]);
  auto timeline = DetachTransition(self.ui, 3, true);
  const auto latest_start = lynx::fml::TimePoint::Now() - expected;
  XCTAssertTrue(timeline->Read(captured));
  XCTAssertEqualWithAccuracy(captured ? *captured : -1, 0.4, 1e-5);
  XCTAssertTrue(timeline->timeline().started);
  XCTAssertTrue(timeline->timeline().start_time >= earliest_start);
  XCTAssertTrue(timeline->timeline().start_time <= latest_start);
  XCTAssertNil([layer animationForKey:key]);
}
@end
