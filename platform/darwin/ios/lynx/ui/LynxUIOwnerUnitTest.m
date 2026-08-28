// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContext+Private.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxView.h>
#import <XCTest/XCTest.h>

#import "LynxUI+Private.h"
#import "LynxUIOwner+Private.h"

@interface LynxExternalMemoryRecordingContext : LynxContext
@property(nonatomic) NSUInteger reportCount;
@property(nonatomic, strong) XCTestExpectation *reportExpectation;
@end

@implementation LynxExternalMemoryRecordingContext

- (void)reportExternalMemoryWithTotalSize:(int64_t)totalSize garbageSize:(int64_t)garbageSize {
  (void)totalSize;
  (void)garbageSize;
  self.reportCount += 1;
  [self.reportExpectation fulfill];
}

@end

@interface LynxUIOwner (ExternalMemoryUnitTest)
@property(nonatomic, readonly) NSSet<NSNumber *> *externalMemoryReportCandidateIds;
@end

@interface LynxUIOwnerUnitTest : XCTestCase
@property(nonatomic, strong) LynxUIOwner *uiOwner;
@end

@implementation LynxUIOwnerUnitTest

- (void)setUp {
  self.uiOwner = [[LynxUIOwner alloc] init];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testGetTagInfo {
  [LynxEnv sharedInstance];

  self.uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                          componentRegistry:[LynxComponentScopeRegistry new]
                                              screenMetrics:nil];

  XCTAssert([self.uiOwner getTagInfo:@"list"] == LynxShadowNodeTypeCommon);
  XCTAssert([self.uiOwner getTagInfo:@"text"] == LynxShadowNodeTypeCustom);
  XCTAssert([self.uiOwner getTagInfo:@"view"] == LynxShadowNodeTypeCommon);
  XCTAssert([self.uiOwner getTagInfo:@"raw-text"] == LynxShadowNodeTypeCustom |
            LynxShadowNodeTypeVirtual);
  XCTAssert([self.uiOwner getTagInfo:@"inline-text"] == LynxShadowNodeTypeCustom |
            LynxShadowNodeTypeVirtual);
  XCTAssert([self.uiOwner getTagInfo:@"xxxx"] == 0);
}

- (void)testExternalMemoryRemovedNodeCandidatesTrackDetachedLifetime {
  LynxComponentScopeRegistry *registry = [LynxComponentScopeRegistry new];
  [LynxComponentScopeRegistry registerBuiltInBehaviors:registry];
  self.uiOwner = [[LynxUIOwner alloc] initWithContainerView:[LynxView new]
                                          componentRegistry:registry
                                              screenMetrics:nil];
  [self.uiOwner createUIWithSign:1
                         tagName:@"page"
                        eventSet:[NSSet set]
                   lepusEventSet:[NSSet set]
                           props:@{}
                       nodeIndex:0
              gestureDetectorSet:[NSSet set]];
  [self.uiOwner createUIWithSign:2
                         tagName:@"view"
                        eventSet:[NSSet set]
                   lepusEventSet:[NSSet set]
                           props:@{}
                       nodeIndex:0
              gestureDetectorSet:[NSSet set]];
  [self.uiOwner createUIWithSign:3
                         tagName:@"view"
                        eventSet:[NSSet set]
                   lepusEventSet:[NSSet set]
                           props:@{}
                       nodeIndex:0
              gestureDetectorSet:[NSSet set]];

  LynxUI *root = [self.uiOwner findUIBySign:1];
  LynxUI *parent = [self.uiOwner findUIBySign:2];
  LynxUI *child = [self.uiOwner findUIBySign:3];

  [self.uiOwner insertNode:2 toParent:1 atIndex:0];
  [self.uiOwner insertNode:3 toParent:2 atIndex:0];
  [self.uiOwner cacheRemovedUIId:2];
  [self.uiOwner onNodeRemoved:2];
  LynxExternalMemorySnapshot attached = [self.uiOwner getExternalMemorySnapshot];
  XCTAssertEqual(attached.garbageSize, 0);

  [self.uiOwner detachNode:2];
  [self.uiOwner cacheRemovedUIId:2];
  [self.uiOwner cacheRemovedUIId:2];
  [self.uiOwner cacheRemovedUIId:3];
  [self.uiOwner onNodeRemoved:2];
  [self.uiOwner onNodeRemoved:3];
  LynxExternalMemorySnapshot detached = [self.uiOwner getExternalMemorySnapshot];
  XCTAssertEqual(detached.totalSize, attached.totalSize);
  XCTAssertEqual(detached.garbageSize, [parent memoryUsageBytes] + [child memoryUsageBytes]);
  XCTAssertEqual(self.uiOwner.externalMemoryReportCandidateIds.count, 2U);
  XCTAssertEqual([self.uiOwner getExternalMemorySnapshot].garbageSize, detached.garbageSize);

  [self.uiOwner insertNode:2 toParent:1 atIndex:0];
  XCTAssertEqual([self.uiOwner getExternalMemorySnapshot].garbageSize, 0);
  XCTAssertEqual(self.uiOwner.externalMemoryReportCandidateIds.count, 2U);

  [self.uiOwner detachNode:2];
  [self.uiOwner cacheRemovedUIId:2];
  [self.uiOwner cacheRemovedUIId:3];
  [self.uiOwner onNodeRemoved:2];
  [self.uiOwner recycleNode:3];
  XCTAssertEqual(self.uiOwner.externalMemoryReportCandidateIds.count, 1U);
  [self.uiOwner recycleNode:2];
  XCTAssertNil([self.uiOwner findUIBySign:2]);
  XCTAssertNil([self.uiOwner findUIBySign:3]);
  XCTAssertEqual(self.uiOwner.externalMemoryReportCandidateIds.count, 0U);
  LynxExternalMemorySnapshot destroyed = [self.uiOwner getExternalMemorySnapshot];
  XCTAssertEqual(destroyed.totalSize, [root memoryUsageBytes]);
  XCTAssertEqual(destroyed.garbageSize, 0);
}

- (void)testExternalMemorySnapshotPrunesMissingCandidates {
  self.uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                          componentRegistry:[LynxComponentScopeRegistry new]
                                              screenMetrics:nil];
  [self.uiOwner cacheRemovedUIId:404];
  [self.uiOwner cacheRemovedUIId:405];
  XCTAssertEqual(self.uiOwner.externalMemoryReportCandidateIds.count, 2U);

  LynxExternalMemorySnapshot snapshot = [self.uiOwner getExternalMemorySnapshot];
  XCTAssertEqual(snapshot.totalSize, 0);
  XCTAssertEqual(snapshot.garbageSize, 0);
  XCTAssertEqual(self.uiOwner.externalMemoryReportCandidateIds.count, 0U);
}

- (void)testExternalMemoryReportRequestIsCoalesced {
  LynxExternalMemoryRecordingContext *context = [LynxExternalMemoryRecordingContext new];
  context.reportExpectation = [self expectationWithDescription:@"external memory report"];
  self.uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                          componentRegistry:[LynxComponentScopeRegistry new]
                                              screenMetrics:nil];
  self.uiOwner.uiContext.lynxContext = context;

  [self.uiOwner requestExternalMemoryReport:0];
  [self.uiOwner requestExternalMemoryReport:0];

  [self waitForExpectations:@[ context.reportExpectation ] timeout:1];
  XCTAssertEqual(context.reportCount, 1U);
}

- (void)testExternalMemoryReportSkipsDestroyedContext {
  LynxExternalMemoryRecordingContext *context = [LynxExternalMemoryRecordingContext new];
  context.hasLynxViewDestroyed = YES;
  context.reportExpectation = [self expectationWithDescription:@"no external memory report"];
  context.reportExpectation.inverted = YES;
  self.uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                          componentRegistry:[LynxComponentScopeRegistry new]
                                              screenMetrics:nil];
  self.uiOwner.uiContext.lynxContext = context;

  [self.uiOwner requestExternalMemoryReport:0];

  [self waitForExpectations:@[ context.reportExpectation ] timeout:0.1];
  XCTAssertEqual(context.reportCount, 0U);
}

@end
