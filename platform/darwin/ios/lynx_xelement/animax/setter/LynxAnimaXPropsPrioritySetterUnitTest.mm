// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXPlayer.h>
#import <XElement/LynxAnimaXPropsPrioritySetter.h>

#import <XCTest/XCTest.h>

@interface LynxAnimaXPropsPrioritySetterTests : XCTestCase
@property(nonatomic, strong) LynxAnimaXPropsPrioritySetter *setter;
@property(nonatomic, strong) NSMutableArray<NSString *> *executedTasks;
@property(nonatomic, strong) AnimaXPlayer *player;
@end
@implementation LynxAnimaXPropsPrioritySetterTests
- (void)setUp {
  [super setUp];
  self.setter = [[LynxAnimaXPropsPrioritySetter alloc] init];
  self.executedTasks = [NSMutableArray array];
  BaseAnimaXAbility *ability = [[BaseAnimaXAbility alloc] init];
  AnimaXContext *ctx = [[AnimaXContext alloc] initWithAbility:ability];
  self.player = [[AnimaXPlayer alloc] initWithContext:ctx];
  [self.setter attachToPlayer:self.player];
}
- (void)tearDown {
  self.setter = nil;
  self.executedTasks = nil;
  self.player = nil;
  [super tearDown];
}

- (void)testExecutionPriorityOrder {
  XCTAssertTrue(AnimaXHigh < AnimaXDefault);
  XCTAssertTrue(AnimaXDefault < AnimaXLow);
}

- (void)testEnqueueTaskAndFlushExecutesTasksInPriorityOrder {
  [self.setter enqueueTask:^(AnimaXPlayer *player) {
    [self.executedTasks addObject:@"default"];
  }];
  [self.setter
      enqueueTask:^(AnimaXPlayer *player) {
        [self.executedTasks addObject:@"high"];
      }
         priority:AnimaXHigh];
  [self.setter
      enqueueTask:^(AnimaXPlayer *player) {
        [self.executedTasks addObject:@"low"];
      }
         priority:AnimaXLow];
  [self.setter flush];
  NSArray<NSString *> *expected = @[ @"high", @"default", @"low" ];
  XCTAssertEqualObjects(self.executedTasks, expected);
}

- (void)testFlushWithNoTasksDoesNotThrowAndNoExecution {
  LynxAnimaXPropsPrioritySetter *s = [[LynxAnimaXPropsPrioritySetter alloc] init];
  [s attachToPlayer:(AnimaXPlayer *)self.player];
  XCTAssertNoThrow([s flush]);
  XCTAssertTrue(self.executedTasks.count == 0);
}

- (void)testFlushWithoutPlayerDoesNothing {
  LynxAnimaXPropsPrioritySetter *s = [[LynxAnimaXPropsPrioritySetter alloc] init];
  __block BOOL executed = NO;
  [s
      enqueueTask:^(AnimaXPlayer *player) {
        executed = YES;
      }
         priority:AnimaXHigh];
  [s flush];
  XCTAssertFalse(executed);
  [s attachToPlayer:(AnimaXPlayer *)self.player];
  [s flush];
  XCTAssertTrue(executed);
}

@end
