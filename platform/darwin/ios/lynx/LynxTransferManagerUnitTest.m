// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxTransferListener.h>
#import <Lynx/LynxTransferWrapperView.h>
#import <Lynx/LynxView.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#import "LynxTransferManager.h"
#import "ui/transfer/LynxUITransfer.h"

@interface LynxView (LynxTransferManagerUnitTest)

- (void)dispatchTransferCreate:(NSString*)transferId
                          view:(UIView*)view
                       dataset:(NSDictionary*)dataset;
- (void)dispatchTransferDatasetUpdate:(UIView*)view dataset:(NSDictionary*)dataset;
- (void)dispatchTransferIntrinsicContentSizeChange:(LynxTransferWrapperView*)view;

@end

@interface LynxUITransfer (LynxTransferManagerUnitTest)

- (LynxView*)lynxView;

@end

@interface LynxTransferTestListener : NSObject <LynxTransferListener>

@property(nonatomic, copy) NSString* acceptedTransferId;
@property(nonatomic, assign) NSInteger createCount;
@property(nonatomic, assign) NSInteger datasetUpdateCount;
@property(nonatomic, assign) NSInteger removeCount;
@property(nonatomic, strong) NSDictionary* createDataset;
@property(nonatomic, strong) NSDictionary* updatedDataset;

@end

@implementation LynxTransferTestListener

- (BOOL)onCreate:(NSString*)transferId view:(UIView*)view dataset:(NSDictionary*)dataset {
  if (![self.acceptedTransferId isEqualToString:transferId]) {
    return NO;
  }
  self.createCount++;
  self.createDataset = dataset;
  return YES;
}

- (void)onDatasetUpdate:(NSString*)transferId dataset:(NSDictionary*)dataset {
  self.datasetUpdateCount++;
  self.updatedDataset = dataset;
}

- (void)onRemove:(NSString*)transferId view:(UIView*)view {
  self.removeCount++;
}

@end

@interface LynxTransferSizeListener : LynxTransferTestListener

@property(nonatomic, assign) NSInteger sizeChangeCount;
@property(nonatomic, strong) LynxTransferWrapperView* lastSizeView;

@end

@implementation LynxTransferSizeListener

- (void)transferViewDidChangeIntrinsicContentSize:(LynxTransferWrapperView*)view {
  self.sizeChangeCount++;
  self.lastSizeView = view;
}

@end

@interface LynxTransferManagerUnitTest : XCTestCase
@end

@implementation LynxTransferManagerUnitTest

- (void)testDatasetLifecycleNotifiesOwner {
  LynxTransferManager* manager = [LynxTransferManager new];
  LynxTransferTestListener* owner = [LynxTransferTestListener new];
  owner.acceptedTransferId = @"accepted";
  LynxTransferTestListener* unrelated = [LynxTransferTestListener new];
  unrelated.acceptedTransferId = @"unrelated";
  UIView* view = [UIView new];
  NSDictionary* initialDataset = @{@"phase" : @"create"};
  NSDictionary* updatedDataset = @{@"phase" : @"update"};

  [manager registerTransferListener:owner];
  [manager registerTransferListener:unrelated];
  [manager dispatchTransferCreate:@"accepted" view:view dataset:initialDataset];
  [manager dispatchTransferDatasetUpdate:view dataset:updatedDataset];
  [manager dispatchTransferRemove:@"accepted" view:view];

  XCTAssertEqual(owner.createCount, 1);
  XCTAssertEqualObjects(owner.createDataset, initialDataset);
  XCTAssertEqual(owner.datasetUpdateCount, 1);
  XCTAssertEqualObjects(owner.updatedDataset, updatedDataset);
  XCTAssertEqual(owner.removeCount, 1);
  XCTAssertEqual(unrelated.createCount, 0);
  XCTAssertEqual(unrelated.datasetUpdateCount, 0);
  XCTAssertEqual(unrelated.removeCount, 0);
}

- (void)testPendingTransferUsesLatestDatasetWhenClaimed {
  LynxTransferManager* manager = [LynxTransferManager new];
  UIView* view = [UIView new];
  NSDictionary* initialDataset = @{@"phase" : @"create"};
  NSDictionary* updatedDataset = @{@"phase" : @"update"};
  [manager dispatchTransferCreate:@"accepted" view:view dataset:initialDataset];
  [manager dispatchTransferDatasetUpdate:view dataset:updatedDataset];

  LynxTransferTestListener* listener = [LynxTransferTestListener new];
  listener.acceptedTransferId = @"accepted";
  [manager registerTransferListener:listener];

  XCTAssertEqual(listener.createCount, 1);
  XCTAssertEqualObjects(listener.createDataset, updatedDataset);
  XCTAssertEqual(listener.datasetUpdateCount, 0);
}

- (void)testIntrinsicContentSizeChangeNotifiesOnlyListenersThatImplementIt {
  LynxTransferManager* manager = [LynxTransferManager new];
  LynxTransferSizeListener* sizeListener = [LynxTransferSizeListener new];
  sizeListener.acceptedTransferId = @"size";
  LynxTransferTestListener* plainListener = [LynxTransferTestListener new];
  plainListener.acceptedTransferId = @"plain";
  LynxTransferWrapperView* sizeWrapper = OCMClassMock([LynxTransferWrapperView class]);
  LynxTransferWrapperView* plainWrapper = OCMClassMock([LynxTransferWrapperView class]);

  [manager registerTransferListener:sizeListener];
  [manager registerTransferListener:plainListener];
  [manager dispatchTransferCreate:@"size" view:sizeWrapper dataset:@{}];
  [manager dispatchTransferCreate:@"plain" view:plainWrapper dataset:@{}];
  [manager dispatchTransferIntrinsicContentSizeChange:sizeWrapper];
  [manager dispatchTransferIntrinsicContentSizeChange:plainWrapper];

  XCTAssertEqual(sizeListener.sizeChangeCount, 1);
  XCTAssertEqual(sizeListener.lastSizeView, sizeWrapper);
}

- (void)testLynxViewForwardsIntrinsicContentSizeChange {
  LynxView* lynxView = [[LynxView alloc] initWithoutRender];
  id manager = OCMClassMock([LynxTransferManager class]);
  [lynxView setValue:manager forKey:@"transferManager"];
  LynxTransferWrapperView* wrapper = OCMClassMock([LynxTransferWrapperView class]);

  [lynxView dispatchTransferIntrinsicContentSizeChange:wrapper];

  OCMVerify([manager dispatchTransferIntrinsicContentSizeChange:wrapper]);
}

- (void)testPropsDidUpdateDispatchesCurrentDataset {
  LynxUITransfer* transfer = [LynxUITransfer new];
  LynxView* lynxView = OCMClassMock([LynxView class]);
  id transferMock = OCMPartialMock(transfer);
  OCMStub([transferMock lynxView]).andReturn(lynxView);
  NSDictionary* initialDataset = @{@"phase" : @"create"};
  NSDictionary* updatedDataset = @{@"phase" : @"update"};
  [LynxPropsProcessor updateProp:@"accepted" withKey:@"transfer-id" forUI:transferMock];
  [LynxPropsProcessor updateProp:initialDataset withKey:@"dataset" forUI:transferMock];

  [transferMock onNodeReady];
  OCMVerify([lynxView dispatchTransferCreate:@"accepted" view:[OCMArg any] dataset:initialDataset]);
  [LynxPropsProcessor updateProp:updatedDataset withKey:@"dataset" forUI:transferMock];
  [transferMock propsDidUpdate];

  OCMVerify([lynxView dispatchTransferDatasetUpdate:[OCMArg any] dataset:updatedDataset]);
}

@end
