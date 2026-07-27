// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import "StaticPageHost+Internal.h"

extern "C" bool LynxStaticPageHostRenderPage(int32_t instanceId);

@interface RecordingStaticPageInstance : NSObject <StaticPageInstance>
@property(nonatomic, strong) id renderData;
@property(nonatomic, strong, nullable) id renderGlobalProps;
@property(nonatomic, strong, nullable) id updatedData;
@property(nonatomic, strong, nullable) id updatedGlobalProps;
@property(nonatomic) BOOL destroyed;
@end

@implementation RecordingStaticPageInstance

- (void)renderPage:(id)data globalProps:(id)globalProps {
  _renderData = data;
  _renderGlobalProps = globalProps;
}

- (void)updateMetaData:(id)data globalProps:(id)globalProps {
  _updatedData = data;
  _updatedGlobalProps = globalProps;
}

- (void)destroy {
  _destroyed = YES;
}

@end

@interface StaticPageHostUnitTest : XCTestCase
@end

@implementation StaticPageHostUnitTest

- (void)testRenderKeepsInitialDataByReference {
  constexpr int32_t kInstanceId = 42;
  NSMutableArray<dispatch_block_t>* tasks = [NSMutableArray array];
  StaticPageHost* host = [[StaticPageHost alloc] initWithTaskDispatcher:^(dispatch_block_t task) {
    [tasks addObject:task];
  }];
  NSDictionary* initialData = @{@"title" : @"initial"};
  RecordingStaticPageInstance* instance = [RecordingStaticPageInstance new];

  [host registerInstanceId:kInstanceId data:initialData globalProps:nil];
  XCTAssertTrue([StaticPageHost attach:kInstanceId instance:instance]);
  XCTAssertTrue(LynxStaticPageHostRenderPage(kInstanceId));
  XCTAssertEqual(initialData, instance.renderData);
  XCTAssertNil(instance.renderGlobalProps);

  [host clear];
  XCTAssertEqual(1u, tasks.count);
  tasks.firstObject();
  XCTAssertTrue(instance.destroyed);
  XCTAssertFalse([StaticPageHost attach:kInstanceId instance:[RecordingStaticPageInstance new]]);
}

- (void)testMetadataMergesBeforeLoadAndDispatchesAfterAttach {
  constexpr int32_t kInstanceId = 43;
  NSMutableArray<dispatch_block_t>* tasks = [NSMutableArray array];
  StaticPageHost* host = [[StaticPageHost alloc] initWithTaskDispatcher:^(dispatch_block_t task) {
    [tasks addObject:task];
  }];
  [host updateMetaData:@{@"before" : @1, @"shared" : @"updated"} globalProps:@{@"theme" : @"dark"}];
  [host updateMetaData:nil globalProps:@{@"theme" : @"dark", @"locale" : @"zh"}];

  NSDictionary* initialData = @{@"initial" : @2, @"shared" : @"initial"};
  NSDictionary* initialGlobalProps = @{@"theme" : @"light"};
  RecordingStaticPageInstance* instance = [RecordingStaticPageInstance new];
  [host registerInstanceId:kInstanceId data:initialData globalProps:initialGlobalProps];
  XCTAssertTrue([StaticPageHost attach:kInstanceId instance:instance]);
  XCTAssertTrue(LynxStaticPageHostRenderPage(kInstanceId));
  XCTAssertEqualObjects((@{@"before" : @1, @"initial" : @2, @"shared" : @"updated"}),
                        instance.renderData);
  XCTAssertEqualObjects(@"dark", instance.renderGlobalProps[@"theme"]);
  XCTAssertEqualObjects(@"zh", instance.renderGlobalProps[@"locale"]);

  NSDictionary* updatedGlobalProps = @{@"theme" : @"dark"};
  [host updateMetaData:@{@"after" : @3} globalProps:updatedGlobalProps];
  XCTAssertEqual(1u, tasks.count);
  XCTAssertNil(instance.updatedData);
  tasks.firstObject();
  XCTAssertEqualObjects((@{@"before" : @1, @"initial" : @2, @"shared" : @"updated", @"after" : @3}),
                        instance.updatedData);
  XCTAssertEqual(updatedGlobalProps, instance.updatedGlobalProps);

  [host clear];
}

- (void)testPendingUpdateIsDroppedWhenHostIsCleared {
  constexpr int32_t kInstanceId = 44;
  NSMutableArray<dispatch_block_t>* tasks = [NSMutableArray array];
  StaticPageHost* host = [[StaticPageHost alloc] initWithTaskDispatcher:^(dispatch_block_t task) {
    [tasks addObject:task];
  }];
  RecordingStaticPageInstance* instance = [RecordingStaticPageInstance new];
  [host registerInstanceId:kInstanceId data:@{} globalProps:nil];
  XCTAssertTrue([StaticPageHost attach:kInstanceId instance:instance]);
  XCTAssertTrue(LynxStaticPageHostRenderPage(kInstanceId));

  [host updateMetaData:@{@"stale" : @YES} globalProps:nil];
  [host clear];

  XCTAssertEqual(2u, tasks.count);
  tasks.firstObject();
  [tasks removeObjectAtIndex:0];
  XCTAssertNil(instance.updatedData);
  tasks.firstObject();
  XCTAssertTrue(instance.destroyed);
}

@end
