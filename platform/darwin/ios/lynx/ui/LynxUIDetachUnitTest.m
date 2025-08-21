// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundManager.h>
#import <Lynx/LynxUI.h>
#import <Lynx/UIView+Lynx.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxUI (Test)
@property(nonatomic, strong) UIView *view;
@property(nonatomic, strong) LynxBackgroundManager *backgroundManager;
@end

@interface LynxUIDetachLayersTests : XCTestCase
@property(nonatomic, strong) LynxUI *lynxUI;
@property(nonatomic, strong) id mockView;
@property(nonatomic, strong) id mockBackgroundManager;
@end

@implementation LynxUIDetachLayersTests

- (void)setUp {
  [super setUp];
  self.lynxUI = [[LynxUI alloc] init];
  self.mockView = OCMClassMock([UIView class]);
  self.mockBackgroundManager = OCMClassMock([LynxBackgroundManager class]);

  self.lynxUI.view = self.mockView;
  [self.lynxUI setValue:self.mockBackgroundManager forKey:@"backgroundManager"];
}

- (void)tearDown {
  self.lynxUI = nil;
  self.mockView = nil;
  self.mockBackgroundManager = nil;
  [super tearDown];
}

- (void)testDetachManagedLayersFromView_WhenViewAndBackgroundManagerExist {
  // Setup
  LynxBackgroundSubBackgroundLayer *mockBgLayer =
      OCMClassMock([LynxBackgroundSubBackgroundLayer class]);
  LynxBorderLayer *mockBorderLayer = OCMClassMock([LynxBorderLayer class]);

  OCMStub([self.mockBackgroundManager relinquishBackgroundLayer]).andReturn(mockBgLayer);
  OCMStub([self.mockBackgroundManager relinquishBorderLayer]).andReturn(mockBorderLayer);

  // Execute
  [self.lynxUI detachManagedLayersFromView];

  // Verify
  OCMVerify([self.mockView setLynxBackgroundLayer:mockBgLayer]);
  OCMVerify([self.mockView setLynxBorderLayer:mockBorderLayer]);
}

- (void)testDetachManagedLayersFromView_WhenViewDoesNotExist {
  // Setup
  self.lynxUI.view = nil;

  // Execute
  [self.lynxUI detachManagedLayersFromView];

  // Verify - No crashes or exceptions should occur
  OCMVerify(never(), [self.mockBackgroundManager relinquishBackgroundLayer]);
  OCMVerify(never(), [self.mockBackgroundManager relinquishBorderLayer]);
}

- (void)testDetachManagedLayersFromView_WhenBackgroundManagerDoesNotExist {
  // Setup
  [self.lynxUI setValue:nil forKey:@"backgroundManager"];

  // Execute
  [self.lynxUI detachManagedLayersFromView];

  // Verify - No crashes or exceptions should occur
  OCMVerify(never(), [self.mockView setLynxBackgroundLayer:[OCMArg any]]);
  OCMVerify(never(), [self.mockView setLynxBorderLayer:[OCMArg any]]);
}

- (void)testDetachManagedLayersFromView_WhenBackgroundManagerReturnsNilLayers {
  // Setup
  OCMStub([self.mockBackgroundManager relinquishBackgroundLayer]).andReturn(nil);
  OCMStub([self.mockBackgroundManager relinquishBorderLayer]).andReturn(nil);

  // Execute
  [self.lynxUI detachManagedLayersFromView];

  // Verify
  OCMVerify([self.mockView setLynxBackgroundLayer:nil]);
  OCMVerify([self.mockView setLynxBorderLayer:nil]);
}

@end
