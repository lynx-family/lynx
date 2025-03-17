// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUIOwner.h>
#import <XCTest/XCTest.h>

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
                                             templateRender:nil
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

@end
