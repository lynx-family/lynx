// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxLogicExecutor.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <XCTest/XCTest.h>

#import "LynxTemplateRender+Protected.h"

@interface LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest;
@end

@implementation LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest {
  return shell_.get();
}
@end

@interface RuntimeTestLogicExecutor : NSObject <LynxLogicExecutor>
@property(nonatomic, strong) NSDictionary *lastEvent;
@end

@implementation RuntimeTestLogicExecutor
- (void)onLynxEvent:(LynxView *)lynxView event:(NSDictionary *)event {
  self.lastEvent = event;
}
- (void)destroy {
}
@end

@interface LynxTemplateRenderRuntimeUnitTest : XCTestCase
@end

@implementation LynxTemplateRenderRuntimeUnitTest

- (void)testHeapSnapshotWritesFileAfterDestroy {
  NSString *path = [NSTemporaryDirectory()
      stringByAppendingPathComponent:[NSUUID.UUID.UUIDString
                                         stringByAppendingString:@".heapsnapshot"]];
  NSError *error = nil;
  XCTAssertTrue([@"previous file content" writeToFile:path
                                           atomically:NO
                                             encoding:NSUTF8StringEncoding
                                                error:&error]);
  XCTAssertNil(error);
  [self addTeardownBlock:^{
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
  }];

  dispatch_semaphore_t allowCapture = dispatch_semaphore_create(0);
  XCTestExpectation *completion = [self expectationWithDescription:@"Snapshot saved"];
  @autoreleasepool {
    LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
      builder.enableJSRuntime = YES;
      builder.enablePendingJSTaskOnLayout = NO;
      builder.backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeQuickjs;
      builder.debuggable = NO;
    }];
    // Queue capture behind a gate. Initialization, capture, and runtime
    // destruction retain their actor order even after the shell is deleted.
    auto *shell = [view.templateRender shellForTest];
    shell->GetRuntimeActor()->ActAsync([allowCapture, self](auto &) {
      XCTAssertEqual(dispatch_semaphore_wait(allowCapture,
                                             dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC)),
                     0);
    });
    [view takeBTSHeapSnapshot:path
                     callback:^(BOOL success) {
                       XCTAssertFalse([NSThread isMainThread]);
                       XCTAssertTrue(success);
                       NSData *data = [NSData dataWithContentsOfFile:path];
                       XCTAssertNotNil(data);
                       if (data != nil) {
                         NSError *parseError = nil;
                         NSDictionary *snapshot =
                             [NSJSONSerialization JSONObjectWithData:data
                                                             options:0
                                                               error:&parseError];
                         XCTAssertNil(parseError);
                         XCTAssertNotNil(snapshot[@"snapshot"]);
                         XCTAssertNotNil(snapshot[@"nodes"]);
                       }
                       [completion fulfill];
                     }];
    [view clearForDestroy];
  }
  // The renderer enqueues shell deletion on main during dealloc. Release the
  // gate on the same serial queue after that deletion has completed.
  dispatch_async(dispatch_get_main_queue(), ^{
    dispatch_semaphore_signal(allowCapture);
  });
  [self waitForExpectationsWithTimeout:15 handler:nil];
}

- (void)testHeapSnapshotRejectsEmbeddedNULWithoutReplacingFile {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    builder.enablePendingJSTaskOnLayout = NO;
    builder.backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeQuickjs;
    builder.debuggable = NO;
  }];
  NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  XCTAssertTrue([@"unchanged" writeToFile:path
                               atomically:NO
                                 encoding:NSUTF8StringEncoding
                                    error:nil]);
  [self addTeardownBlock:^{
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
  }];
  const unichar suffix[] = {0, 'x'};
  NSString *invalidPath = [path stringByAppendingString:[NSString stringWithCharacters:suffix
                                                                                length:2]];
  XCTestExpectation *completion = [self expectationWithDescription:@"Embedded NUL rejected"];
  [view takeBTSHeapSnapshot:invalidPath
                   callback:^(BOOL success) {
                     XCTAssertFalse(success);
                     XCTAssertFalse([NSThread isMainThread]);
                     [completion fulfill];
                   }];
  [self waitForExpectationsWithTimeout:5 handler:nil];
  XCTAssertEqualObjects([NSString stringWithContentsOfFile:path
                                                  encoding:NSUTF8StringEncoding
                                                     error:nil],
                        @"unchanged");
}

- (void)testHeapSnapshotRejectsInvalidPathsOnBackgroundThread {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    builder.enablePendingJSTaskOnLayout = NO;
    builder.backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeQuickjs;
    builder.debuggable = NO;
  }];
  for (NSString *path in @[
         @"", @"relative.heapsnapshot", @"~/snapshot.heapsnapshot", @"~user/snapshot.heapsnapshot"
       ]) {
    XCTAssertFalse([view.templateRender takeBTSHeapSnapshot:path callback:nil]);
    XCTestExpectation *completion = [self expectationWithDescription:@"Invalid snapshot path"];
    [view takeBTSHeapSnapshot:path
                     callback:^(BOOL success) {
                       XCTAssertFalse(success);
                       XCTAssertFalse([NSThread isMainThread]);
                       [completion fulfill];
                     }];
  }
  [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testHeapSnapshotRejectsDisabledRuntime {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = NO;
  }];
  XCTestExpectation *completion = [self expectationWithDescription:@"Disabled runtime"];
  NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:@"disabled.heapsnapshot"];
  [view takeBTSHeapSnapshot:path
                   callback:^(BOOL success) {
                     XCTAssertFalse(success);
                     XCTAssertFalse([NSThread isMainThread]);
                     [completion fulfill];
                   }];
  [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testHeapSnapshotFromBackgroundThreadAfterDestroy {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = NO;
  }];
  [view clearForDestroy];
  XCTestExpectation *completion = [self expectationWithDescription:@"Destroyed view"];
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
    [view takeBTSHeapSnapshot:@"/unused.heapsnapshot"
                     callback:^(BOOL success) {
                       XCTAssertFalse(success);
                       XCTAssertFalse([NSThread isMainThread]);
                       [completion fulfill];
                     }];
    // A completion is optional, including on the failure path.
    [view takeBTSHeapSnapshot:@"/unused.heapsnapshot" callback:nil];
  });
  [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testLogicExecutorPreservesGlobalEventsWithoutPerViewRuntime {
  RuntimeTestLogicExecutor *executor = [RuntimeTestLogicExecutor new];
  LynxViewGroup *group = [[LynxViewGroup alloc] initWithUrl:@"test://embedded"
                                             templateBundle:[LynxTemplateBundle new]];
  group.enableJSRuntime = YES;
  group.logicExecutor = executor;
  [group setEmbeddedMode:static_cast<LynxEmbeddedMode>(5)];
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.lynxViewGroup = group;
  }];

  auto *shell = [view.templateRender shellForTest];
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue([view getLynxContext].enableJSRuntime);
  XCTAssertTrue(shell->IsRuntimeEnabled());
  XCTAssertTrue(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
  XCTAssertFalse(shell->GetTasm()->ShouldSendEventToMainThread());
  XCTAssertTrue(shell->GetTasm()->ShouldPostDataToJs(nullptr));
  auto config = std::make_shared<lynx::tasm::PageConfig>();
  shell->GetTasm()->SetPageConfig(config);
  for (auto mode :
       {lynx::tasm::AIR_MODE_OFF, lynx::tasm::AIR_MODE_FIBER, lynx::tasm::AIR_MODE_STRICT}) {
    config->SetLynxAirMode(mode);
    XCTAssertEqual(shell->GetTasm()->ShouldPostDataToJs(nullptr), mode == lynx::tasm::AIR_MODE_OFF);
    XCTAssertFalse(shell->GetTasm()->ShouldSendEventToMainThread());
  }
  [view sendGlobalEvent:@"runtime-test-event" withParams:@[ @"payload" ]];
  XCTAssertEqualObjects(executor.lastEvent[@"method"], kSendGlobalEvent);
  XCTAssertEqualObjects(executor.lastEvent[@"name"], @"runtime-test-event");
  XCTAssertEqualObjects(executor.lastEvent[@"params"], @[ @"payload" ]);
}

- (void)testEmbeddedModeWithoutLogicExecutorKeepsRuntime {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    [builder setEmbeddedMode:static_cast<LynxEmbeddedMode>(5)];
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
  auto config = std::make_shared<lynx::tasm::PageConfig>();
  shell->GetTasm()->SetPageConfig(config);
  for (auto mode :
       {lynx::tasm::AIR_MODE_OFF, lynx::tasm::AIR_MODE_FIBER, lynx::tasm::AIR_MODE_STRICT}) {
    config->SetLynxAirMode(mode);
    XCTAssertTrue(shell->GetTasm()->ShouldPostDataToJs(nullptr));
  }
}

- (void)testRegularViewKeepsRuntime {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
}

- (void)testDisabledRuntimeStaysDisabled {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = NO;
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertFalse(view.templateRender.enableJSRuntime);
  XCTAssertFalse(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
}

- (void)testAirStrictModeKeepsRuntimeDisabled {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    builder.enableAirStrictMode = YES;
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertFalse(view.templateRender.enableJSRuntime);
  XCTAssertFalse(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
}

@end
