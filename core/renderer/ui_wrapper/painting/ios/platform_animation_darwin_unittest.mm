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

gfx::PlatformAnimationCommand OpacityCommand(uint32_t generation = 1, long duration = 10000,
                                             int iterations = 1) {
  gfx::PlatformAnimationCommand command;
  command.animation_id = 1;
  command.generation = generation;
  command.name = "typed";
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

void Apply(LynxUI* ui, gfx::PlatformAnimationCommand command) {
  auto commands = std::make_shared<gfx::PlatformAnimationCommandBatch>();
  commands->push_back(std::move(command));
  tasm::ApplyPlatformAnimationCommands(ui, commands);
}
}  // namespace

@interface ResizingTypedKeyframeAnimator : LynxKeyframeAnimator
@property(nonatomic) NSUInteger rawParseCount;
@end
@implementation ResizingTypedKeyframeAnimator
- (BOOL)shouldReInitTransform {
  return YES;
}
- (void)parseKeyframes:(LynxAnimationInfo*)info {
  ++_rawParseCount;
}
@end

@interface PlatformAnimationDarwinTest : XCTestCase
@property(nonatomic, strong) LynxUI* ui;
@end

@implementation PlatformAnimationDarwinTest
- (void)setUp {
  self.ui = [[LynxUI alloc] initWithView:[[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)]];
  self.ui.view.layer.opacity = 0.4;
}

- (void)tearDown {
  [self.ui.animationManager endAllAnimation];
  [self.ui.view.layer removeAllAnimations];
  self.ui = nil;
}

- (void)drainMainQueue {
  XCTestExpectation* expectation = [self expectationWithDescription:@"animation completion"];
  dispatch_async(dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:1];
}

- (void)testZeroDurationCompletesWithFillAndEvents {
  id events = OCMClassMock([LynxAnimationDelegate class]);
  OCMExpect([events sendAnimationEvent:self.ui
                             eventName:@"animationstart"
                           eventParams:[OCMArg any]]);
  OCMExpect([events sendAnimationEvent:self.ui eventName:@"animationend" eventParams:[OCMArg any]]);
  Apply(self.ui, OpacityCommand(1, 0));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.8, 0.001);
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
  XCTAssertEqual(self.ui.view.layer.animationKeys.count, 0u);
  OCMVerifyAll(events);
  [events stopMocking];
  events = OCMClassMock([LynxAnimationDelegate class]);
  OCMReject([events sendAnimationEvent:self.ui eventName:[OCMArg any] eventParams:[OCMArg any]]);
  Apply(self.ui, OpacityCommand(2, 0));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.8, 0.001);
  [events stopMocking];
}

- (void)testZeroIterationsUsesInitialEndpoint {
  Apply(self.ui, OpacityCommand(1, 10000, 0));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.2, 0.001);
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
}

- (void)testRunningAnimationUpdatedToZeroRemovesOldAnimation {
  Apply(self.ui, OpacityCommand());
  XCTAssertNotNil([self.ui.view.layer animationForKey:@"opacitytyped"]);
  Apply(self.ui, OpacityCommand(2, 0));
  [self drainMainQueue];
  XCTAssertNil([self.ui.view.layer animationForKey:@"opacitytyped"]);
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.8, 0.001);
}

- (void)testZeroDurationWithoutFillRestoresUnderlyingStyle {
  auto command = OpacityCommand(1, 0);
  command.animation_data.fill_mode = gfx::AnimationFillModeType::kNone;
  Apply(self.ui, std::move(command));
  [self drainMainQueue];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.4, 0.001);
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
  CAAnimation* animation = [self.ui.view.layer animationForKey:@"opacitytyped"];
  XCTAssertNotNil(animation);
  [animation.delegate animationDidStop:animation finished:YES];
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
}

- (void)testCanceledDelayedZeroDurationCannotFinishLater {
  auto command = OpacityCommand(1, 0);
  command.animation_data.delay = 20;
  command.animation_data.fill_mode = gfx::AnimationFillModeType::kBoth;
  Apply(self.ui, std::move(command));
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.2, 0.001);
  auto cancel = OpacityCommand(2);
  cancel.type = gfx::PlatformAnimationCommandType::kCancel;
  Apply(self.ui, std::move(cancel));
  id events = OCMClassMock([LynxAnimationDelegate class]);
  OCMReject([events sendAnimationEvent:self.ui eventName:@"animationend" eventParams:[OCMArg any]]);
  XCTestExpectation* expectation = [self expectationWithDescription:@"canceled delay"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 40 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:1];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.4, 0.001);
  [events stopMocking];
}

- (void)testReverseForwardsFillUsesOriginalStart {
  auto command = OpacityCommand();
  command.animation_data.direction = gfx::AnimationDirectionType::kReverse;
  Apply(self.ui, std::move(command));
  CAKeyframeAnimation* animation =
      (CAKeyframeAnimation*)[self.ui.view.layer animationForKey:@"opacitytyped"];
  XCTAssertEqualWithAccuracy([animation.values.firstObject doubleValue], 0.8, 0.001);
  [animation.delegate animationDidStop:animation finished:YES];
  XCTAssertEqualWithAccuracy(self.ui.view.layer.opacity, 0.2, 0.001);
}

- (void)testNegativeFullRotationKeepsRotationChannel {
  auto command = OpacityCommand();
  command.properties.clear();
  gfx::PlatformAnimationProperty property;
  property.property = gfx::AnimationPropertyType::kTransform;
  for (int i = 0; i < 2; ++i) {
    auto keyframe = gfx::TransformKeyframe::Create(lynx::fml::TimeDelta::FromSeconds(i));
    gfx::TransformOperations operations;
    operations.AppendRotate(gfx::TransformOperation::Type::kRotateZ, i * -360.f);
    keyframe->SetResolvedValue(std::move(operations));
    property.keyframes.push_back(std::move(keyframe));
  }
  command.properties.push_back(std::move(property));
  Apply(self.ui, std::move(command));
  CAKeyframeAnimation* rotation =
      (CAKeyframeAnimation*)[self.ui.view.layer animationForKey:@"transform.rotation.ztyped"];
  XCTAssertNotNil(rotation);
  XCTAssertEqualWithAccuracy([rotation.values.lastObject doubleValue], -2 * M_PI, 0.001);
}

- (void)testResetForegroundAndListReuseResumeTypedAnimations {
  Apply(self.ui, OpacityCommand());
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  [self.ui.animationManager resetAnimation];
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
  [self.ui.animationManager resumeAnimation];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  [self.ui.animationManager resetAnimation];
  [self.ui.animationManager restartAnimation];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  [self.ui.animationManager detachFromUI];
  XCTAssertFalse([self.ui.animationManager hasAnimationRunning]);
  [self.ui.animationManager attachToUI:self.ui];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
}

- (void)testReorderedEffectsPreservePlatformPrecedence {
  auto commands = std::make_shared<gfx::PlatformAnimationCommandBatch>();
  for (int i = 1; i <= 3; ++i) {
    auto command = OpacityCommand();
    command.animation_id = i;
    command.name = "fade" + std::to_string(i);
    commands->push_back(std::move(command));
  }
  tasm::ApplyPlatformAnimationCommands(self.ui, commands);
  NSArray* initial = @[ @"opacityfade1", @"opacityfade2", @"opacityfade3" ];
  XCTAssertEqualObjects(self.ui.view.layer.animationKeys, initial);
  commands->clear();
  for (int i = 3; i >= 1; --i) {
    auto command = OpacityCommand(2);
    command.animation_id = i;
    command.name = "fade" + std::to_string(i);
    commands->push_back(std::move(command));
  }
  tasm::ApplyPlatformAnimationCommands(self.ui, commands);
  NSArray* reordered = @[ @"opacityfade3", @"opacityfade2", @"opacityfade1" ];
  XCTAssertEqualObjects(self.ui.view.layer.animationKeys, reordered);
  [self.ui.animationManager resetAnimation];
  [self.ui.animationManager resumeAnimation];
  XCTAssertEqualObjects(self.ui.view.layer.animationKeys, reordered);
}

- (void)testResizeRebuildsTypedValuesWithoutRawParser {
  ResizingTypedKeyframeAnimator* animator =
      [[ResizingTypedKeyframeAnimator alloc] initWithUI:self.ui];
  LynxAnimationInfo* info = [[LynxAnimationInfo alloc] initWithName:@"resize"];
  info.duration = 10;
  __block NSUInteger builds = 0;
  LynxTypedKeyframesProvider provider = ^LynxKeyframeParsedData*(LynxUI* ui) {
    ++builds;
    LynxKeyframeParsedData* parsed = [LynxKeyframeParsedData new];
    parsed.keyframeValues[@"opacity"] = [@[ @0.2, @0.8 ] mutableCopy];
    parsed.keyframeTimes[@"opacity"] = [@[ @0, @1 ] mutableCopy];
    parsed.beginStyles[@"opacity"] = @0.2;
    parsed.endStyles[@"opacity"] = @0.8;
    return parsed;
  };
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:NO generation:1];
  NSUInteger initialBuilds = builds;
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:YES generation:2];
  XCTAssertGreaterThan(builds, initialBuilds);
  initialBuilds = builds;
  [animator reapply];
  XCTAssertGreaterThan(builds, initialBuilds);
  XCTAssertEqual(animator.rawParseCount, 0u);
  XCTAssertTrue([animator isRunning]);
  [animator destroy];
}

- (void)testPlaybackAndOrderUpdatesReuseParsedKeyframes {
  LynxKeyframeAnimator* animator = [[LynxKeyframeAnimator alloc] initWithUI:self.ui];
  LynxAnimationInfo* info = [[LynxAnimationInfo alloc] initWithName:@"reuse"];
  info.duration = 10;
  info.iterationCount = 1;
  info.playState = LynxAnimationPlayStateRunning;
  __block NSUInteger builds = 0;
  LynxTypedKeyframesProvider provider = ^LynxKeyframeParsedData*(LynxUI* ui) {
    ++builds;
    LynxKeyframeParsedData* parsed = [LynxKeyframeParsedData new];
    parsed.keyframeValues[@"opacity"] = [@[ @0.2, @0.8 ] mutableCopy];
    parsed.keyframeTimes[@"opacity"] = [@[ @0, @1 ] mutableCopy];
    parsed.beginStyles[@"opacity"] = @0.2;
    parsed.endStyles[@"opacity"] = @0.8;
    return parsed;
  };
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:NO generation:1];
  info = [info copy];
  info.playState = LynxAnimationPlayStatePaused;
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:YES generation:2];
  XCTAssertEqual([[animator valueForKey:@"state"] unsignedIntegerValue],
                 (NSUInteger)LynxKFAnimatorStatePaused);
  info = [info copy];
  info.playState = LynxAnimationPlayStateRunning;
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:YES generation:3];
  XCTAssertTrue([animator isRunning]);
  // An order-only update still reapplies the animator without converting values.
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:YES generation:4];
  XCTAssertEqual(builds, 1u);
  [animator applyAnimationInfo:info keyframesProvider:provider reuseKeyframes:NO generation:5];
  XCTAssertEqual(builds, 2u);
  [animator destroy];
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
  auto command = OpacityCommand();
  command.properties.clear();
  gfx::PlatformAnimationProperty property;
  property.property = gfx::AnimationPropertyType::kTransform;
  for (int i = 0; i < 2; ++i) {
    auto frame = gfx::TransformKeyframe::Create(lynx::fml::TimeDelta::FromSeconds(i));
    frame->SetResolvedValue(operations);
    property.keyframes.push_back(std::move(frame));
  }
  command.properties.push_back(std::move(property));
  Apply(self.ui, std::move(command));
  CAKeyframeAnimation* animation =
      (CAKeyframeAnimation*)[self.ui.view.layer animationForKey:@"transformtyped"];
  XCTAssertNotNil(animation);
  CATransform3D actual = [animation.values.firstObject CATransform3DValue];
  XCTAssertTrue(CATransform3DEqualToTransform(actual, expected));
}

- (void)testOldKeyframeCompletionCannotFinishNewGeneration {
  Apply(self.ui, OpacityCommand());
  CAAnimation* previous = [self.ui.view.layer animationForKey:@"opacitytyped"];
  Apply(self.ui, OpacityCommand(2, 20000));
  [previous.delegate animationDidStop:previous finished:YES];
  XCTAssertTrue([self.ui.animationManager hasAnimationRunning]);
  XCTAssertNotNil([self.ui.view.layer animationForKey:@"opacitytyped"]);
}

- (void)testRetargetedTransitionRejectsOldEventsAndCleanup {
  auto first = OpacityCommand();
  first.kind = gfx::AnimationKind::kTransition;
  Apply(self.ui, std::move(first));
  LynxTransitionAnimationManager* manager = self.ui.transitionAnimationManager;
  LynxAnimationDelegate* previous = [manager valueForKey:@"transitionDelegates"][@(OPACITY)];
  DidAnimationStart oldStart = previous.didStart;
  DidAnimationStop oldStop = previous.didStop;
  auto next = OpacityCommand(2);
  next.kind = gfx::AnimationKind::kTransition;
  Apply(self.ui, std::move(next));
  LynxAnimationDelegate* current = [manager valueForKey:@"transitionDelegates"][@(OPACITY)];
  XCTAssertNotNil(current);
  XCTAssertNotEqual(previous, current);
  id events = OCMClassMock([LynxAnimationDelegate class]);
  OCMReject([events sendAnimationEvent:self.ui eventName:[OCMArg any] eventParams:[OCMArg any]]);
  XCTAssertNotNil(oldStart);
  XCTAssertNotNil(oldStop);
  if (oldStart) oldStart(nil);
  if (oldStop) oldStop(nil, YES);
  XCTAssertEqual([manager valueForKey:@"transitionDelegates"][@(OPACITY)], current);
  [events stopMocking];
  [current forceStop];
}
@end
