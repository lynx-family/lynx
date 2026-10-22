// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxAnimationDelegate.h>
#import <Lynx/LynxConverter+Transform.h>
#import <Lynx/LynxPlatformAnimation.h>
#import <Lynx/LynxUI+Internal.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#include "core/renderer/ui_wrapper/painting/ios/platform_animation_darwin.h"

namespace {
using namespace lynx;
using EventState = gfx::AnimationPlaybackState::EventState;
NSString* const kAnimationName = @"TestAnimation";

gfx::PlatformAnimationCommand OpacityCommand(uint32_t generation = 1, long duration = 10000,
                                             int iterations = 1) {
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
    keyframe->SetValue(i == 0 ? 0.2f : 0.8f);
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
  auto commands = std::make_shared<gfx::PlatformAnimationCommandBatch>();
  commands->push_back(std::move(command));
  tasm::ApplyPlatformAnimationCommands(ui, commands);
}
}  // namespace

// Keep CAAnimation copies, but deliver callbacks explicitly so Core Animation
// cannot finish animations while the test drains the main queue.
@interface ManuallyDrivenAnimationLayer : CALayer
@property(nonatomic, strong) NSMutableDictionary<NSString*, CAAnimation*>* storedAnimations;
@end
@implementation ManuallyDrivenAnimationLayer
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
  auto playback = std::make_shared<gfx::AnimationPlaybackState>(
      lynx::fml::TimeDelta::FromSeconds(3), true, EventState::kStarted);
  auto command = OpacityCommand();
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
    command.playback = std::make_shared<gfx::AnimationPlaybackState>(
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
  auto playback = std::make_shared<gfx::AnimationPlaybackState>(
      lynx::fml::TimeDelta::FromSeconds(3), true, EventState::kStarted);
  const auto executor = playback->ChangeExecutor();
  auto command = OpacityCommand();
  command.playback = playback;
  command.executor = executor;
  Apply(self.ui, std::move(command));
  CAAnimation* previous = [self animationForKeyPath:@"opacity"];
  auto update = OpacityCommand(2, 20000);
  update.playback = playback;
  update.executor = executor;
  Apply(self.ui, std::move(update));
  [previous.delegate animationDidStop:previous finished:YES];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  XCTAssertNotNil([self animationForKeyPath:@"opacity"]);

  OCMExpect([self.events sendAnimationEvent:self.ui
                                  eventName:@"animationcancel"
                                eventParams:[OCMArg any]]);
  auto cancel = OpacityCommand(2, 20000);
  cancel.type = gfx::PlatformAnimationCommandType::kCancel;
  cancel.playback = playback;
  cancel.executor = executor;
  Apply(self.ui, std::move(cancel));
  OCMVerifyAll(self.events);
  XCTAssertTrue(playback->event_state() == EventState::kCanceled);
  XCTAssertNil([self animationForKeyPath:@"opacity"]);
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

- (void)testRunningAnimationUpdatedToZeroRemovesOldAnimation {
  Apply(self.ui, OpacityCommand());
  XCTAssertNotNil([self animationForKeyPath:@"opacity"]);
  Apply(self.ui, OpacityCommand(2, 0));
  [self drainMainQueue];
  XCTAssertNil([self animationForKeyPath:@"opacity"]);
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.8, 0.001);
}

- (void)testPausedZeroDurationCompletesOnlyAfterResume {
  auto paused = OpacityCommand(1, 0);
  paused.animation_data.play_state = gfx::AnimationPlayStateType::kPaused;
  Apply(self.ui, std::move(paused));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.4, 0.001);
  Apply(self.ui, OpacityCommand(2, 0));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.8, 0.001);
}

- (void)testRunningUpdateBeforeDeferredInitialPauseRemainsRunning {
  auto paused = OpacityCommand();
  paused.animation_data.play_state = gfx::AnimationPlayStateType::kPaused;
  Apply(self.ui, std::move(paused));
  auto running = OpacityCommand(2);
  running.reuse_keyframes = true;
  Apply(self.ui, std::move(running));
  [self drainMainQueue];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
}

- (void)testInitialPauseRunsOnMainQueue {
  auto paused = OpacityCommand();
  paused.animation_data.play_state = gfx::AnimationPlayStateType::kPaused;
  Apply(self.ui, std::move(paused));
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  [self drainMainQueue];
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
  XCTAssertNil([self animationForKeyPath:@"opacity"]);
  auto running = OpacityCommand(2);
  running.reuse_keyframes = true;
  Apply(self.ui, std::move(running));
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  XCTAssertNotNil([self animationForKeyPath:@"opacity"]);
}

- (void)testCancelInvalidatesPendingInstantCompletion {
  OCMReject([self.events sendAnimationEvent:self.ui
                                  eventName:@"animationend"
                                eventParams:[OCMArg any]]);
  Apply(self.ui, OpacityCommand(1, 0));
  auto cancel = OpacityCommand(2);
  cancel.type = gfx::PlatformAnimationCommandType::kCancel;
  Apply(self.ui, std::move(cancel));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.4, 0.001);
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
}

- (void)testNegativeFullRotationKeepsRotationChannel {
  gfx::TransformOperations operations;
  operations.AppendRotate(gfx::TransformOperation::kRotateZ, -360.f);
  Apply(self.ui, TransformCommand({}, operations));
  CAKeyframeAnimation* rotation = [self animationForKeyPath:@"transform.rotation.z"];
  XCTAssertNotNil(rotation);
  XCTAssertEqualWithAccuracy([rotation.values.lastObject doubleValue], -2 * M_PI, 0.001);
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
  auto command = TransformCommand(operations, operations);
  auto opacity = OpacityCommand();
  command.properties.push_back(std::move(opacity.properties.front()));
  Apply(self.ui, std::move(command));
  CAKeyframeAnimation* animation = [self animationForKeyPath:@"transform"];
  XCTAssertNotNil(animation);
  XCTAssertTrue(
      CATransform3DEqualToTransform([animation.values.firstObject CATransform3DValue], expected));
  XCTAssertEqualWithAccuracy([[self animationForKeyPath:@"opacity"].values.lastObject doubleValue],
                             0.8, 0.001);
}
@end
