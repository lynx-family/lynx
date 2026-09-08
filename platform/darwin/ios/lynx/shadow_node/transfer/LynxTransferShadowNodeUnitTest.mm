// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLayoutNodeManager.h>
#import <Lynx/LynxTextShadowNode.h>
#import <Lynx/LynxUIImage.h>
#import <XCTest/XCTest.h>

#import "LynxTransferShadowNode.h"

@interface LynxTransferTestLayoutNodeManager : LynxLayoutNodeManager

@property(nonatomic, strong) NSMutableArray<NSNumber*>* measuredSigns;
@property(nonatomic, strong) NSMutableArray<NSNumber*>* alignedSigns;

@end

@implementation LynxTransferTestLayoutNodeManager

- (instancetype)init {
  self = [super initWithNativePtr:nullptr];
  if (self) {
    _measuredSigns = [NSMutableArray new];
    _alignedSigns = [NSMutableArray new];
  }
  return self;
}

- (MeasureResult)measureWithSign:(NSInteger)sign
                    MeasureParam:(MeasureParam*)param
                  MeasureContext:(MeasureContext*)context {
  [self.measuredSigns addObject:@(sign)];
  return (MeasureResult){CGSizeZero, 0.f};
}

- (void)alignWithSign:(NSInteger)sign
           AlignParam:(AlignParam*)param
         AlignContext:(AlignContext*)context {
  [self.alignedSigns addObject:@(sign)];
}

@end

@interface LynxTransferShadowNodeUnitTest : XCTestCase
@end

@implementation LynxTransferShadowNodeUnitTest

- (void)testMeasuresAndAlignsCustomChildren {
  LynxTransferShadowNode* transfer = [[LynxTransferShadowNode alloc] initWithSign:1
                                                                          tagName:@"transfer-view"];
  LynxTextShadowNode* text = [[LynxTextShadowNode alloc] initWithSign:2 tagName:@"text"];
  LynxImageShadowNode* image = [[LynxImageShadowNode alloc] initWithSign:3 tagName:@"image"];
  [transfer insertChild:text atIndex:0];
  [transfer insertChild:image atIndex:1];

  LynxTransferTestLayoutNodeManager* manager = [LynxTransferTestLayoutNodeManager new];
  transfer.layoutNodeManager = manager;
  MeasureParam* measureParam = [[MeasureParam alloc] initWithWidth:100
                                                         WidthMode:LynxMeasureModeDefinite
                                                            Height:200
                                                        HeightMode:LynxMeasureModeDefinite];

  [transfer customMeasureLayoutNode:measureParam measureContext:[MeasureContext new]];
  [transfer customAlignLayoutNode:[AlignParam new] alignContext:[AlignContext new]];

  XCTAssertEqualObjects(manager.measuredSigns, (@[ @2, @3 ]));
  XCTAssertEqualObjects(manager.alignedSigns, (@[ @2, @3 ]));
}

@end
