// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIExposureUnitTest.h"
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIScroller.h>
#import <Lynx/LynxUIView.h>

@implementation LynxUIExposureUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testStopExposure {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  NSMutableSet<LynxUIExposureDetail *> *set = OCMClassMock([NSMutableSet class]);
  mockExposure.uiInWindowMapBefore = set;

  [mockExposure stopExposure:@{@"sendEvent" : @NO}];
  OCMVerify([mockExposure removeFromRunLoop]);
  OCMVerify(never(), [mockExposure sendEvent:[OCMArg any] eventName:@"disexposure"]);
  OCMVerify(never(), [set removeAllObjects]);

  [mockExposure stopExposure:@{@"sendEvent" : @YES}];
  OCMVerify(times(2), [mockExposure removeFromRunLoop]);
  OCMVerify([mockExposure sendEvent:[OCMArg any] eventName:@"disexposure"]);
  OCMVerify([set removeAllObjects]);
}

- (void)testResumeExposure {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  NSMutableDictionary<NSString *, LynxUIExposureDetail *> *map =
      OCMPartialMock([NSMutableDictionary dictionary]);
  mockExposure.exposedLynxUIMap = map;
  OCMStub([map count]).andReturn(2);
  [mockExposure resumeExposure];
  OCMVerify([mockExposure addExposureToRunLoop]);
}

- (void)testAddExposureToRunLoop {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  NSMutableDictionary<NSString *, LynxUIExposureDetail *> *map =
      OCMPartialMock([NSMutableDictionary dictionary]);
  mockExposure.exposedLynxUIMap = map;
  OCMStub([map count]).andReturn(2);
  [mockExposure stopExposure:@{@"sendEvent" : @YES}];
  [mockExposure addExposureToRunLoop];
  XCTAssertNil(mockExposure.displayLink);
  [mockExposure resumeExposure];
  XCTAssertNotNil(mockExposure.displayLink);
}

- (void)testDisableExposureDetection {
  LynxUIExposure *exposure = [[LynxUIExposure alloc] init];
  [exposure setEnableExposureDetection:NO];

  LynxUI *ui = [[LynxUI alloc] init];
  ui.sign = 1;
  ui.exposureID = @"1";
  [exposure addLynxUI:ui withUniqueIdentifier:nil extraData:nil useOptions:nil];
  [exposure addExposureToRunLoop];

  XCTAssertEqual(exposure.exposedLynxUIMap.count, 0);
  XCTAssertNil(exposure.displayLink);
}

- (void)testAddLynxUI {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  NSMutableDictionary<NSString *, LynxUIExposureDetail *> *map =
      OCMPartialMock([NSMutableDictionary dictionary]);
  mockExposure.exposedLynxUIMap = map;
  LynxUIContext *mockCtx = OCMPartialMock([[LynxUIContext alloc] init]);
  mockCtx.uiExposure = mockExposure;

  LynxUI *ui = [[LynxUI alloc] init];
  [ui setRawEvents:[NSSet setWithArray:@[ @"uiappear(bindEvent)" ]] andLepusRawEvents:nil];
  ui.sign = 1;
  ui.context = mockCtx;
  [LynxPropsProcessor updateProp:@"1" withKey:@"exposure-id" forUI:ui];
  [ui propsDidUpdate];
  [ui performSelector:@selector(onNodeReadyForUIOwner)];

  // Add LynxUI, for global event and custom event
  OCMVerify([mockExposure addExposureToRunLoop]);
  XCTAssertTrue([map count] == 2);
}

- (void)testRemoveLynxUI {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  NSMutableDictionary<NSString *, LynxUIExposureDetail *> *map =
      OCMPartialMock([NSMutableDictionary dictionary]);
  mockExposure.exposedLynxUIMap = map;
  LynxUIContext *mockCtx = OCMPartialMock([[LynxUIContext alloc] init]);
  mockCtx.uiExposure = mockExposure;

  LynxUI *ui = [[LynxUI alloc] init];
  [ui setRawEvents:[NSSet setWithArray:@[ @"uiappear(bindEvent)" ]] andLepusRawEvents:nil];
  ui.sign = 1;
  ui.context = mockCtx;
  [LynxPropsProcessor updateProp:@"1" withKey:@"exposure-id" forUI:ui];
  [ui propsDidUpdate];
  [ui performSelector:@selector(onNodeReadyForUIOwner)];

  // Remove LynxUI, for global event and custom event
  [mockCtx removeUIFromExposedMap:ui withUniqueIdentifier:nil];
  OCMVerify(times(2), [mockExposure removeFromRunLoop]);
  XCTAssertTrue([map count] == 0);
}

- (void)testOverlayExposure {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  LynxRootUI *mockRoot =
      OCMPartialMock([[LynxRootUI alloc] initWithLynxView:(LynxView *)[UIView new]]);
  OCMStub([mockRoot isVisible]).andReturn(YES);
  OCMStub([mockRoot getBoundingClientRectToScreen]).andReturn(CGRectMake(0, 400, 300, 100));
  mockExposure.rootUI = mockRoot;
  LynxUIView *mockOverlay = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  OCMStub([mockOverlay isVisible]).andReturn(YES);
  OCMExpect([mockOverlay isOverlay]).andReturn(NO);
  LynxUIView *mockUI = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  OCMStub([mockUI isVisible]).andReturn(YES);
  LynxUIView *mockExposeTarget = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  mockExposeTarget.exposureID = @"1";
  OCMStub([mockExposeTarget isVisible]).andReturn(YES);
  OCMStub([mockExposeTarget getBoundingClientRectToScreen]).andReturn(CGRectMake(0, 0, 300, 50));
  OCMStub([mockExposure borderOfExposureScreen:mockExposeTarget])
      .andReturn(CGRectMake(0, 0, 300, 500));

  [mockRoot insertChild:mockOverlay atIndex:0];
  [mockOverlay insertChild:mockUI atIndex:0];
  [mockUI insertChild:mockExposeTarget atIndex:0];

  mockExposeTarget.frame = CGRectMake(0, 0, 300, 50);
  [mockExposure addLynxUI:mockExposeTarget withUniqueIdentifier:nil extraData:nil useOptions:nil];
  [mockExposure stopExposure:@{@"sendEvent" : @YES}];
  mockExposure.isStopExposure = NO;

  [mockExposure exposureHandler:nil];
  XCTAssertTrue(mockExposure.uiInWindowMapBefore.count == 0);
  OCMExpect([mockOverlay isOverlay]).andReturn(YES);
  [mockExposure exposureHandler:nil];
  XCTAssertTrue(mockExposure.uiInWindowMapBefore.count == 1);
  OCMVerifyAll(mockOverlay);
}

- (void)testClipExposure {
  LynxUIExposure *mockExposure = OCMPartialMock([[LynxUIExposure alloc] init]);
  LynxRootUI *mockRoot =
      OCMPartialMock([[LynxRootUI alloc] initWithLynxView:(LynxView *)[UIView new]]);
  OCMStub([mockRoot isVisible]).andReturn(YES);
  OCMStub([mockRoot getBoundingClientRectToScreen]).andReturn(CGRectMake(0, 0, 300, 500));
  mockExposure.rootUI = mockRoot;
  LynxUIView *mockScrollUI =
      OCMPartialMock([[LynxUIScroller alloc] initWithView:[UIScrollView new]]);
  OCMStub([mockScrollUI isVisible]).andReturn(YES);
  OCMStub([mockScrollUI getBoundingClientRectToScreen]).andReturn(CGRectMake(0, 0, 300, 100));
  LynxUIView *mockClipUI = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  OCMStub([mockClipUI isVisible]).andReturn(YES);
  mockClipUI.enableExposureUIClip = kLynxEventPropEnable;
  OCMStub([mockClipUI getBoundingClientRectToScreen]).andReturn(CGRectMake(0, 150, 300, 100));
  LynxUIView *mockExposeTarget = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  mockExposeTarget.exposureID = @"1";
  OCMStub([mockExposeTarget isVisible]).andReturn(YES);
  OCMStub([mockExposeTarget getBoundingClientRectToScreen]).andReturn(CGRectMake(0, 300, 300, 50));
  OCMStub([mockExposure borderOfExposureScreen:mockExposeTarget])
      .andReturn(CGRectMake(0, 0, 300, 500));

  [mockRoot insertChild:mockScrollUI atIndex:0];
  [mockScrollUI insertChild:mockClipUI atIndex:0];
  [mockClipUI insertChild:mockExposeTarget atIndex:0];

  mockExposeTarget.frame = CGRectMake(0, 150, 300, 50);
  [mockExposure addLynxUI:mockExposeTarget withUniqueIdentifier:nil extraData:nil useOptions:nil];
  [mockExposure stopExposure:@{@"sendEvent" : @YES}];
  mockExposure.isStopExposure = NO;

  [mockExposure exposureHandler:nil];
  XCTAssertTrue(mockExposure.uiInWindowMapBefore.count == 0);
  mockScrollUI.enableExposureUIClip = kLynxEventPropDisable;
  [mockExposure exposureHandler:nil];
  XCTAssertTrue(mockExposure.uiInWindowMapBefore.count == 0);
  mockClipUI.enableExposureUIClip = kLynxEventPropDisable;
  [mockExposure exposureHandler:nil];
  XCTAssertTrue(mockExposure.uiInWindowMapBefore.count == 1);
}

@end
