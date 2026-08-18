// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxUIRendererProtocol.h>
#import <Lynx/LynxView+Internal.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxUITreeHelper.h>
#import <OCMock/OCMock.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

@interface DevToolInsertTextField : UITextField
@end

@implementation DevToolInsertTextField

- (BOOL)isFirstResponder {
  return YES;
}

@end

// These fakes intentionally implement only the UI tree selectors under test.
// Use NSObject's real respondsToSelector: behavior instead of stubbing it with OCMock.
@interface DevToolUITreeRendererFake : NSObject
@property(nonatomic, copy) NSString *tree;
@property(nonatomic, copy) NSString *nodeInfo;
@property(nonatomic, assign) int styleResult;
@property(nonatomic, assign) NSUInteger treeCalls;
@property(nonatomic, assign) NSUInteger nodeInfoCalls;
@property(nonatomic, assign) NSUInteger styleCalls;
@property(nonatomic, assign) int nodeInfoId;
@property(nonatomic, assign) int styleNodeId;
@property(nonatomic, copy) NSString *styleName;
@property(nonatomic, copy) NSString *styleContent;
- (NSString *)recordTree;
- (NSString *)recordNodeInfo:(int)nodeId;
- (int)recordStyle:(int)nodeId name:(NSString *)name content:(NSString *)content;
@end

@implementation DevToolUITreeRendererFake
- (NSString *)recordTree {
  self.treeCalls++;
  return self.tree;
}
- (NSString *)recordNodeInfo:(int)nodeId {
  self.nodeInfoCalls++;
  self.nodeInfoId = nodeId;
  return self.nodeInfo;
}
- (int)recordStyle:(int)nodeId name:(NSString *)name content:(NSString *)content {
  self.styleCalls++;
  self.styleNodeId = nodeId;
  self.styleName = name;
  self.styleContent = content;
  return self.styleResult;
}
@end

@interface DevToolTreeOnlyRendererFake : DevToolUITreeRendererFake
- (NSString *)getLynxUITree;
@end
@implementation DevToolTreeOnlyRendererFake
- (NSString *)getLynxUITree {
  return [self recordTree];
}
@end

@interface DevToolNodeInfoOnlyRendererFake : DevToolUITreeRendererFake
- (NSString *)getUINodeInfo:(int)nodeId;
@end
@implementation DevToolNodeInfoOnlyRendererFake
- (NSString *)getUINodeInfo:(int)nodeId {
  return [self recordNodeInfo:nodeId];
}
@end

@interface DevToolStyleOnlyRendererFake : DevToolUITreeRendererFake
- (int)setUIStyle:(int)nodeId withStyleName:(NSString *)name withStyleContent:(NSString *)content;
@end
@implementation DevToolStyleOnlyRendererFake
- (int)setUIStyle:(int)nodeId withStyleName:(NSString *)name withStyleContent:(NSString *)content {
  return [self recordStyle:nodeId name:name content:content];
}
@end

@interface DevToolFullUITreeRendererFake : DevToolTreeOnlyRendererFake
- (NSString *)getUINodeInfo:(int)nodeId;
- (int)setUIStyle:(int)nodeId withStyleName:(NSString *)name withStyleContent:(NSString *)content;
@end
@implementation DevToolFullUITreeRendererFake
- (NSString *)getUINodeInfo:(int)nodeId {
  return [self recordNodeInfo:nodeId];
}
- (int)setUIStyle:(int)nodeId withStyleName:(NSString *)name withStyleContent:(NSString *)content {
  return [self recordStyle:nodeId name:name content:content];
}
@end

@interface DevToolPlatformDarwinDelegateUnitTest : XCTestCase
@end

@implementation DevToolPlatformDarwinDelegateUnitTest

- (void)testClayUITreeOperationsUseRenderer {
  id view = OCMClassMock([LynxView class]);
  id render = OCMClassMock([LynxTemplateRender class]);
  DevToolFullUITreeRendererFake *renderer = [DevToolFullUITreeRendererFake new];
  OCMStub([view templateRender]).andReturn(render);
  OCMStub([render lynxUIRenderer]).andReturn(renderer);
  NSString *tree = @"{\"name\":\"page\",\"label\":\"hello 🌍\"}";
  NSString *info = @"{\"id\":14}";
  renderer.tree = tree;
  renderer.nodeInfo = info;
  renderer.styleResult = 0;
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:view];
  // Reject any fallback to the native UI tree for a Clay renderer.
  id helper = OCMStrictClassMock([LynxUITreeHelper class]);
  [platform setValue:helper forKey:@"uiTreeHelper"];

  XCTAssertEqualObjects([platform getLynxUITree], tree);
  XCTAssertEqualObjects([platform getUINodeInfo:14], info);
  XCTAssertEqual([platform setUIStyle:14 withStyleName:@"opacity" withStyleContent:@"0.5"], 0);
  XCTAssertEqual(renderer.treeCalls, 1u);
  XCTAssertEqual(renderer.nodeInfoCalls, 1u);
  XCTAssertEqual(renderer.nodeInfoId, 14);
  XCTAssertEqual(renderer.styleCalls, 1u);
  XCTAssertEqual(renderer.styleNodeId, 14);
  XCTAssertEqualObjects(renderer.styleName, @"opacity");
  XCTAssertEqualObjects(renderer.styleContent, @"0.5");
}

- (void)checkUITreeRoutingWithRenderer:(DevToolUITreeRendererFake *)renderer
                                  tree:(BOOL)hasTree
                              nodeInfo:(BOOL)hasNodeInfo
                                 style:(BOOL)hasStyle {
  id view = OCMClassMock([LynxView class]);
  id render = OCMClassMock([LynxTemplateRender class]);
  OCMStub([view templateRender]).andReturn(render);
  OCMStub([render lynxUIRenderer]).andReturn(renderer);
  XCTAssertEqual([renderer respondsToSelector:@selector(getLynxUITree)], hasTree);
  XCTAssertEqual([renderer respondsToSelector:@selector(getUINodeInfo:)], hasNodeInfo);
  XCTAssertEqual([renderer respondsToSelector:@selector(setUIStyle:
                                                     withStyleName:withStyleContent:)],
                 hasStyle);
  id helper = OCMStrictClassMock([LynxUITreeHelper class]);
  NSString *tree = @"{\"name\":\"LynxRootUI\"}";
  NSString *info = @"{\"id\":14}";
  renderer.tree = tree;
  renderer.nodeInfo = info;
  renderer.styleResult = 0;
  if (!hasTree) {
    OCMStub([helper getLynxUITree]).andReturn(tree);
  }
  if (!hasNodeInfo) {
    OCMStub([helper getUINodeInfo:14]).andReturn(info);
  }
  if (!hasStyle) {
    OCMStub([helper setUIStyle:14 withStyleName:@"opacity" withStyleContent:@"0.5"]).andReturn(0);
  }
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:view];
  [platform setValue:helper forKey:@"uiTreeHelper"];

  XCTAssertEqualObjects([platform getLynxUITree], tree);
  XCTAssertEqualObjects([platform getUINodeInfo:14], info);
  XCTAssertEqual([platform setUIStyle:14 withStyleName:@"opacity" withStyleContent:@"0.5"], 0);
  XCTAssertEqual(renderer.treeCalls, hasTree ? 1u : 0u);
  XCTAssertEqual(renderer.nodeInfoCalls, hasNodeInfo ? 1u : 0u);
  XCTAssertEqual(renderer.styleCalls, hasStyle ? 1u : 0u);
  if (!hasTree) {
    OCMVerify([helper getLynxUITree]);
  }
  if (hasNodeInfo) {
    XCTAssertEqual(renderer.nodeInfoId, 14);
  } else {
    OCMVerify([helper getUINodeInfo:14]);
  }
  if (hasStyle) {
    XCTAssertEqual(renderer.styleNodeId, 14);
    XCTAssertEqualObjects(renderer.styleName, @"opacity");
    XCTAssertEqualObjects(renderer.styleContent, @"0.5");
  } else {
    OCMVerify([helper setUIStyle:14 withStyleName:@"opacity" withStyleContent:@"0.5"]);
  }
}

- (void)testNativeRendererKeepsUITreeHelper {
  [self checkUITreeRoutingWithRenderer:[DevToolUITreeRendererFake new]
                                  tree:NO
                              nodeInfo:NO
                                 style:NO];
}

- (void)testClayWithoutUITreeDoesNotFallBackToNativeHelper {
  id view = OCMClassMock([LynxView class]);
  id render = OCMClassMock([LynxTemplateRender class]);
  DevToolFullUITreeRendererFake *renderer = [DevToolFullUITreeRendererFake new];
  OCMStub([view templateRender]).andReturn(render);
  OCMStub([render lynxUIRenderer]).andReturn(renderer);
  renderer.styleResult = -1;
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:view];
  id helper = OCMStrictClassMock([LynxUITreeHelper class]);
  [platform setValue:helper forKey:@"uiTreeHelper"];

  XCTAssertNil([platform getLynxUITree]);
  XCTAssertNil([platform getUINodeInfo:14]);
  XCTAssertEqual([platform setUIStyle:14 withStyleName:@"opacity" withStyleContent:@"0.5"], -1);
  XCTAssertEqual(renderer.treeCalls, 1u);
  XCTAssertEqual(renderer.nodeInfoCalls, 1u);
  XCTAssertEqual(renderer.styleCalls, 1u);
}

- (void)testRendererWithOnlyTreeFallsBackForOtherOperations {
  [self checkUITreeRoutingWithRenderer:[DevToolTreeOnlyRendererFake new]
                                  tree:YES
                              nodeInfo:NO
                                 style:NO];
}

- (void)testRendererWithOnlyNodeInfoFallsBackForOtherOperations {
  [self checkUITreeRoutingWithRenderer:[DevToolNodeInfoOnlyRendererFake new]
                                  tree:NO
                              nodeInfo:YES
                                 style:NO];
}

- (void)testRendererWithOnlyStyleFallsBackForOtherOperations {
  [self checkUITreeRoutingWithRenderer:[DevToolStyleOnlyRendererFake new]
                                  tree:NO
                              nodeInfo:NO
                                 style:YES];
}

- (void)testMissingViewReturnsEmptyUITreeResults {
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];
  [platform setValue:nil forKey:@"uiTreeHelper"];
  XCTAssertNil([platform getLynxUITree]);
  XCTAssertNil([platform getUINodeInfo:14]);
  XCTAssertEqual([platform setUIStyle:14 withStyleName:@"opacity" withStyleContent:@"0.5"], -1);
}

- (void)testInsertTextUsesFirstResponderTextInput {
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectZero];
  UIView *container = [[UIView alloc] initWithFrame:CGRectZero];
  DevToolInsertTextField *textField = [[DevToolInsertTextField alloc] initWithFrame:CGRectZero];
  textField.text = @"ac";
  textField.selectedTextRange = [textField
      textRangeFromPosition:[textField positionFromPosition:textField.beginningOfDocument offset:1]
                 toPosition:[textField positionFromPosition:textField.beginningOfDocument
                                                     offset:1]];
  [container addSubview:textField];
  [lynxView addSubview:container];

  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:(LynxView *)lynxView];

  [platform insertText:@"b"];

  XCTAssertEqualObjects(textField.text, @"abc");
}

@end
