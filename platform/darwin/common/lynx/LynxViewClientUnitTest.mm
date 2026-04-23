// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxFrameView.h>
#import <Lynx/LynxLifecycleDispatcher.h>
#import <Lynx/LynxPerformanceController.h>
#import <Lynx/LynxPerformanceEntryConverter.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxUIFrame.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewClientV2.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>
#import "LynxUnitTestUtils.h"
#include "core/services/timing_handler/timing_constants.h"

@interface LynxViewClientV2Tester : NSObject <LynxViewLifecycleV2>

@property(readwrite) LynxView *lynxView;

@property(readwrite) XCTestExpectation *onPageStartedExpectation;
@property(readwrite) NSArray *onPageStartedExpectationArray;

- (instancetype)initWithLynxView:(nullable LynxView *)view;

@end

@implementation LynxViewClientV2Tester

- (instancetype)initWithLynxView:(nullable LynxView *)view {
  self = [super init];
  if (self) {
    [self setLynxView:view];
  }
  return self;
}

- (void)onPageStartedWithLynxView:(nonnull LynxView *)lynxView
                 withPipelineInfo:(nonnull LynxPipelineInfo *)info {
  XCTAssertTrue(_lynxView == lynxView);
  XCTAssertEqualObjects([_lynxView url], [info url]);
  static int count = 0;
  XCTAssertTrue([_onPageStartedExpectationArray[count++] integerValue] & [info pipelineOrigin]);
  [_onPageStartedExpectation fulfill];
}

@end

@interface LynxPerformanceEventClientTester : NSObject <LynxViewLifecycleV2>

@property(nonatomic, assign) NSInteger performanceEventCount;
@property(nonatomic, strong, nullable) LynxPerformanceEntry *lastEntry;

@end

@implementation LynxPerformanceEventClientTester

- (void)onPerformanceEvent:(nonnull LynxPerformanceEntry *)entry {
  self.performanceEventCount += 1;
  self.lastEntry = entry;
}

@end

@interface LynxFrameView (PerformanceTesting)

- (BOOL)ensureRenderCreated;
- (LynxLifecycleDispatcher *)getLifecycleDispatcher;
- (void)onFrameLoadMetricsEvent:(nonnull LynxPerformanceEntry *)entry;
- (void)setIntrinsicContentSize:(CGSize)size;

@end

@interface LynxCustomEventCapturingEmitter : LynxEventEmitter

@property(nonatomic, strong, nullable) LynxCustomEvent *lastEvent;
@property(nonatomic, assign) NSUInteger eventCount;

@end

@implementation LynxCustomEventCapturingEmitter

- (void)sendCustomEvent:(LynxCustomEvent *)event {
  self.eventCount += 1;
  self.lastEvent = event;
}

@end

@interface LynxViewClientUnitTest : LynxUnitTest

@end

@implementation LynxViewClientUnitTest

- (LynxFrameView *)frameViewWithRootView:(LynxView *)rootView
                                    sign:(NSInteger)sign
                                eventSet:(NSSet<NSString *> *)eventSet
                                 emitter:(LynxCustomEventCapturingEmitter *)emitter {
  LynxUIOwner *uiOwner = [rootView.templateRender uiOwner];
  [uiOwner createUIWithSign:sign
                    tagName:@"frame"
                   eventSet:eventSet
              lepusEventSet:nil
                      props:@{}
                  nodeIndex:0
         gestureDetectorSet:nil];

  uiOwner.uiContext.eventEmitter = emitter;

  LynxUIFrame *uiFrame = (LynxUIFrame *)[uiOwner findUIBySign:sign];
  LynxFrameView *frameView = uiFrame.view;
  [frameView setUrl:@"lynx://frame"];
  return frameView;
}

- (LynxTemplateRender *)ensureRenderForFrameView:(LynxFrameView *)frameView {
  XCTAssertTrue([frameView ensureRenderCreated]);
  LynxTemplateRender *render = [frameView valueForKey:@"_render"];
  XCTAssertNotNil(render);
  return render;
}

- (void)setUp {
  [super setUp];
}

- (void)testAddLynxLifecycle {
  LynxViewClientV2Tester *clientA = [[LynxViewClientV2Tester alloc] initWithLynxView:nil];
  LynxViewClientV2Tester *clientB = [[LynxViewClientV2Tester alloc] initWithLynxView:nil];

  [[LynxEnv sharedInstance].lifecycleDispatcher addLifecycleClient:clientA];
  LynxView *lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  [lynxView addLifecycleClient:clientB];

  NSSet *expectedClients = [NSSet setWithObjects:clientA, clientB, nil];
  NSMutableSet *rawClients = [NSMutableSet set];
  [[[lynxView getLifecycleDispatcher] lifecycleClients]
      enumerateObjectsUsingBlock:^(id _Nonnull client, NSUInteger idx, BOOL *_Nonnull stop) {
        if ([client isKindOfClass:LynxLifecycleDispatcher.class]) {
          [rawClients addObjectsFromArray:[client lifecycleClients]];
        } else {
          [rawClients addObject:client];
        }
      }];
  XCTAssertEqualObjects(expectedClients, rawClients);
}

- (void)testOnPageStarted {
  LynxView *lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];

  LynxViewClientV2Tester *client = [[LynxViewClientV2Tester alloc] initWithLynxView:lynxView];
  XCTestExpectation *onPageStartedExpectation = [self expectationWithDescription:@"onPageStarted"];
  [onPageStartedExpectation setExpectedFulfillmentCount:3];
  [client setOnPageStartedExpectation:onPageStartedExpectation];
  [client
      setOnPageStartedExpectationArray:@[ @(LynxFirstScreen), @(LynxReload), @(LynxFirstScreen) ]];
  [lynxView addLifecycleClient:client];

  NSString *url = @"hello-world.js";
  NSData *data = [LynxUnitTest getNSData:url];
  [lynxView loadTemplate:data withURL:url];
  [lynxView reloadTemplateWithTemplateData:[[LynxTemplateData alloc] initWithDictionary:@{}]];
  [lynxView loadTemplate:data withURL:url];

  [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testTemplateRenderDispatchesPerformanceEventToContainerDispatcher {
  LynxView *lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];

  LynxPerformanceEventClientTester *containerClient =
      [[LynxPerformanceEventClientTester alloc] init];
  [lynxView addLifecycleClient:containerClient];

  LynxTemplateRender *render = [lynxView templateRender];

  NSDictionary *entry = @{@"entryType" : @"pipeline", @"name" : @"loadBundle"};
  [(id<TemplateRenderCallbackProtocol>)render onPerformanceEvent:entry];

  XCTAssertEqual(containerClient.performanceEventCount, 1);
  XCTAssertEqualObjects(containerClient.lastEntry.name, @"loadBundle");
}

- (void)testFrameTemplateRenderPerformanceDispatchStaysLocalToFrame {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];

  LynxPerformanceEventClientTester *rootClient = [[LynxPerformanceEventClientTester alloc] init];
  LynxPerformanceEventClientTester *frameClientA = [[LynxPerformanceEventClientTester alloc] init];
  LynxPerformanceEventClientTester *frameClientB = [[LynxPerformanceEventClientTester alloc] init];
  [rootView addLifecycleClient:rootClient];

  LynxFrameView *frameViewA = [[LynxFrameView alloc] init];
  [frameViewA initWithRootView:rootView];
  LynxTemplateRender *renderA = [[LynxTemplateRender alloc] initWithBuilderBlock:nil
                                                                   containerView:frameViewA];
  [[frameViewA getLifecycleDispatcher] addLifecycleClient:frameClientA];

  LynxFrameView *frameViewB = [[LynxFrameView alloc] init];
  [frameViewB initWithRootView:rootView];
  LynxTemplateRender *renderB = [[LynxTemplateRender alloc] initWithBuilderBlock:nil
                                                                   containerView:frameViewB];
  [[frameViewB getLifecycleDispatcher] addLifecycleClient:frameClientB];

  NSDictionary *entry = @{@"entryType" : @"pipeline", @"name" : @"loadBundle"};
  [(id<TemplateRenderCallbackProtocol>)renderA onPerformanceEvent:entry];

  XCTAssertEqual(rootClient.performanceEventCount, 0);
  XCTAssertEqual(frameClientA.performanceEventCount, 1);
  XCTAssertEqual(frameClientB.performanceEventCount, 0);

  [(id<TemplateRenderCallbackProtocol>)renderB onPerformanceEvent:entry];

  XCTAssertEqual(rootClient.performanceEventCount, 0);
  XCTAssertEqual(frameClientA.performanceEventCount, 1);
  XCTAssertEqual(frameClientB.performanceEventCount, 1);
}

- (void)testFrameViewDispatchesLoadMetricsCustomEventWhenBound {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:100
                                                eventSet:[NSSet setWithObject:@"loadmetrics"]
                                                 emitter:emitter];
  [frameView setEmbeddedMode:LynxEmbeddedModeBase];

  NSDictionary *entryDict = @{
    @"entryType" : @"pipeline",
    @"name" : @"loadBundle",
    @"paintEnd" : @42,
  };
  LynxPerformanceEntry *entry = [LynxPerformanceEntryConverter makePerformanceEntry:entryDict];

  [frameView onFrameLoadMetricsEvent:entry];

  XCTAssertNotNil(emitter.lastEvent);
  XCTAssertEqualObjects(emitter.lastEvent.eventName, @"loadmetrics");
  XCTAssertEqual(emitter.lastEvent.targetSign, 100);
  XCTAssertEqualObjects([emitter.lastEvent paramsName], @"detail");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"url"], @"lynx://frame");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"mode"], @"embedded");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"entry"], entryDict);
}

- (void)testFrameViewSkipsLoadMetricsCustomEventWhenUnbound {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:101
                                                eventSet:[NSSet set]
                                                 emitter:emitter];

  LynxPerformanceEntry *entry = [LynxPerformanceEntryConverter
      makePerformanceEntry:@{@"entryType" : @"pipeline", @"name" : @"loadBundle"}];

  [frameView onFrameLoadMetricsEvent:entry];

  XCTAssertNil(emitter.lastEvent);
}

- (void)testFrameViewDispatchesLoadMetricsCustomEventThroughRenderLifecycle {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:102
                                                eventSet:[NSSet setWithObject:@"loadmetrics"]
                                                 emitter:emitter];

  LynxTemplateRender *render = [self ensureRenderForFrameView:frameView];
  [(id<TemplateRenderCallbackProtocol>)render
      onPerformanceEvent:@{@"entryType" : @"pipeline", @"name" : @"updateTriggeredByNative"}];
  XCTAssertNil(emitter.lastEvent);

  NSDictionary *entryDict = @{
    @"entryType" : @"pipeline",
    @"name" : @"loadBundle",
    @"paintEnd" : @42,
  };
  [(id<TemplateRenderCallbackProtocol>)render onPerformanceEvent:entryDict];

  XCTAssertNotNil(emitter.lastEvent);
  XCTAssertEqualObjects(emitter.lastEvent.eventName, @"loadmetrics");
  XCTAssertEqual(emitter.lastEvent.targetSign, 102);
  XCTAssertEqualObjects([emitter.lastEvent paramsName], @"detail");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"url"], @"lynx://frame");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"mode"], @"standard");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"entry"], entryDict);
}

- (void)testFrameViewEmbeddedModeBitmaskEnablesEmbeddedLoadMetricsEvent {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:103
                                                eventSet:[NSSet setWithObject:@"loadmetrics"]
                                                 emitter:emitter];
  [frameView setEmbeddedMode:(LynxEmbeddedModeBase | LynxEmbeddedModeEnginePool)];

  LynxTemplateRender *render = [self ensureRenderForFrameView:frameView];
  LynxPerformanceController *controller = [render performanceController];
  XCTAssertNotNil(controller);

  [controller setTiming:1000 key:@(lynx::tasm::timing::kLoadBundleStart) pipelineID:nil];
  XCTAssertNil(emitter.lastEvent);

  [controller setTiming:2000 key:@(lynx::tasm::timing::kPaintEnd) pipelineID:nil];

  XCTAssertNotNil(emitter.lastEvent);
  XCTAssertEqualObjects(emitter.lastEvent.eventName, @"loadmetrics");
  XCTAssertEqual(emitter.lastEvent.targetSign, 103);
  XCTAssertEqualObjects([emitter.lastEvent paramsName], @"detail");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"url"], @"lynx://frame");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"mode"], @"embedded");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"entry"][@"entryType"], @"pipeline");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"entry"][@"name"], @"loadBundle");
}

- (void)testFrameViewDispatchesLayoutChangeCustomEventWhenIntrinsicSizeChanges {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:104
                                                eventSet:[NSSet setWithObject:@"layoutchange"]
                                                 emitter:emitter];

  [frameView setIntrinsicContentSize:CGSizeMake(321, 181)];

  XCTAssertNotNil(emitter.lastEvent);
  XCTAssertEqual(emitter.eventCount, 1u);
  XCTAssertEqualObjects(emitter.lastEvent.eventName, @"layoutchange");
  XCTAssertEqual(emitter.lastEvent.targetSign, 104);
  XCTAssertEqualObjects([emitter.lastEvent paramsName], @"detail");
  XCTAssertNotNil(emitter.lastEvent.params[@"id"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"dataset"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"left"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"top"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"right"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"bottom"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"width"]);
  XCTAssertNotNil(emitter.lastEvent.params[@"height"]);
  XCTAssertEqualObjects(emitter.lastEvent.params[@"frameUrl"], @"lynx://frame");
  XCTAssertEqualObjects(emitter.lastEvent.params[@"intrinsicWidth"], @321);
  XCTAssertEqualObjects(emitter.lastEvent.params[@"intrinsicHeight"], @181);
}

- (void)testFrameViewSkipsLayoutChangeCustomEventWhenUnbound {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:105
                                                eventSet:[NSSet set]
                                                 emitter:emitter];

  [frameView setIntrinsicContentSize:CGSizeMake(321, 181)];

  XCTAssertNil(emitter.lastEvent);
  XCTAssertEqual(emitter.eventCount, 0u);
}

- (void)testFrameViewSkipsDuplicateLayoutChangeCustomEventForSameIntrinsicSize {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
  }];
  LynxCustomEventCapturingEmitter *emitter = [[LynxCustomEventCapturingEmitter alloc] init];
  LynxFrameView *frameView = [self frameViewWithRootView:rootView
                                                    sign:106
                                                eventSet:[NSSet setWithObject:@"layoutchange"]
                                                 emitter:emitter];

  [frameView setIntrinsicContentSize:CGSizeMake(321, 181)];
  [frameView setIntrinsicContentSize:CGSizeMake(321, 181)];

  XCTAssertNotNil(emitter.lastEvent);
  XCTAssertEqual(emitter.eventCount, 1u);
  XCTAssertEqualObjects(emitter.lastEvent.eventName, @"layoutchange");
}
@end
