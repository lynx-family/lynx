// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxView.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#import "LynxUIOwner+Private.h"

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

- (void)testInitialPositionChangeListenerRegistration {
  LynxComponentScopeRegistry *registry = [LynxComponentScopeRegistry new];
  [LynxComponentScopeRegistry registerBuiltInBehaviors:registry];
  LynxUIOwner *owner = [[LynxUIOwner alloc] initWithContainerView:[LynxView new]
                                                componentRegistry:registry
                                                    screenMetrics:nil];
  id rectCache = OCMStrictClassMock([NSMapTable class]);
  [owner setValue:rectCache forKey:@"lastPositionChangeRects"];
  NSSet<NSString *> *responseChainEvents = [NSSet setWithObject:@"positionchange(bindEvent)"];
  NSSet<NSString *> *globalBindEvents = [NSSet setWithObject:@"positionchange(global-bindEvent)"];

  [owner createUIWithSign:1
                  tagName:@"view"
                 eventSet:responseChainEvents
            lepusEventSet:nil
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];
  [owner createUIWithSign:2
                  tagName:@"view"
                 eventSet:globalBindEvents
            lepusEventSet:nil
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];
  [owner createUIWithSign:3
                  tagName:@"view"
                 eventSet:nil
            lepusEventSet:[NSSet setWithObject:@"positionchange(catchEvent)"]
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];
  [owner createUIWithSign:4
                  tagName:@"view"
                 eventSet:nil
            lepusEventSet:nil
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];

  NSSet<NSNumber *> *listeners = [owner valueForKey:@"positionChangeListeners"];
  XCTAssertTrue([listeners containsObject:@1]);
  XCTAssertFalse([listeners containsObject:@2]);
  XCTAssertTrue([listeners containsObject:@3]);
  XCTAssertFalse([listeners containsObject:@4]);
}

- (void)testResolvedClassCreationRegistersPositionChangeListener {
  LynxUIOwner *owner = [[LynxUIOwner alloc] initWithContainerView:[LynxView new]
                                                componentRegistry:nil
                                                    screenMetrics:nil];
  id rectCache = OCMStrictClassMock([NSMapTable class]);
  [owner setValue:rectCache forKey:@"lastPositionChangeRects"];

  [owner createUISyncWithSign:1
                      tagName:@"view"
                        clazz:[LynxUIView class]
               supportedState:LynxSupportedTag
                     eventSet:[NSSet setWithObject:@"positionchange(bindEvent)"]
                lepusEventSet:nil
                        props:nil
                    nodeIndex:0
           gestureDetectorSet:nil];

  [owner createUISyncWithSign:2
                      tagName:@"view"
                        clazz:[LynxUIView class]
               supportedState:LynxSupportedTag
                     eventSet:nil
                lepusEventSet:nil
                        props:nil
                    nodeIndex:0
           gestureDetectorSet:nil];

  NSSet<NSNumber *> *listeners = [owner valueForKey:@"positionChangeListeners"];
  XCTAssertTrue([listeners containsObject:@1]);
  XCTAssertFalse([listeners containsObject:@2]);
}

- (void)testPositionChangeListenerUpdateRemovesDynamicUnbind {
  LynxComponentScopeRegistry *registry = [LynxComponentScopeRegistry new];
  [LynxComponentScopeRegistry registerBuiltInBehaviors:registry];
  LynxUIOwner *owner = [[LynxUIOwner alloc] initWithContainerView:[LynxView new]
                                                componentRegistry:registry
                                                    screenMetrics:nil];
  [owner createUIWithSign:1
                  tagName:@"view"
                 eventSet:[NSSet setWithObject:@"positionchange(bindEvent)"]
            lepusEventSet:nil
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];

  LynxUI *ui = [owner findUIBySign:1];
  NSMapTable<LynxUI *, NSValue *> *rectCache = [owner valueForKey:@"lastPositionChangeRects"];
  [rectCache setObject:[NSValue valueWithCGRect:CGRectMake(1, 2, 3, 4)] forKey:ui];
  XCTAssertNotNil([owner valueForKey:@"positionChangeObserver"]);

  [owner updateUIWithSign:1
                    props:nil
                 eventSet:[NSSet set]
            lepusEventSet:nil
       gestureDetectorSet:nil];

  NSSet<NSNumber *> *listeners = [owner valueForKey:@"positionChangeListeners"];
  XCTAssertFalse([listeners containsObject:@1]);
  XCTAssertNil([rectCache objectForKey:ui]);
  XCTAssertNil([owner valueForKey:@"positionChangeObserver"]);
  XCTAssertFalse([[owner valueForKey:@"positionChangeDispatchPending"] boolValue]);
}

- (void)testUnregisteredPositionChangeListenerSkipsCleanup {
  LynxComponentScopeRegistry *registry = [LynxComponentScopeRegistry new];
  [LynxComponentScopeRegistry registerBuiltInBehaviors:registry];
  LynxUIOwner *owner = [[LynxUIOwner alloc] initWithContainerView:[LynxView new]
                                                componentRegistry:registry
                                                    screenMetrics:nil];
  [owner createUIWithSign:1
                  tagName:@"view"
                 eventSet:nil
            lepusEventSet:nil
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];
  id rectCache = OCMStrictClassMock([NSMapTable class]);
  [owner setValue:rectCache forKey:@"lastPositionChangeRects"];

  [owner updateUIWithSign:1
                    props:nil
                 eventSet:[NSSet setWithObject:@"positionchange(global-bindEvent)"]
            lepusEventSet:nil
       gestureDetectorSet:nil];
  [owner recycleNode:1];

  XCTAssertEqual([[owner valueForKey:@"positionChangeListeners"] count], 0U);
  XCTAssertNil([owner valueForKey:@"positionChangeObserver"]);
  XCTAssertNil([owner findUIBySign:1]);
}

- (void)testRecycledPositionChangeListenerClearsRectCache {
  LynxComponentScopeRegistry *registry = [LynxComponentScopeRegistry new];
  [LynxComponentScopeRegistry registerBuiltInBehaviors:registry];
  LynxUIOwner *owner = [[LynxUIOwner alloc] initWithContainerView:[LynxView new]
                                                componentRegistry:registry
                                                    screenMetrics:nil];
  [owner createUIWithSign:1
                  tagName:@"view"
                 eventSet:[NSSet setWithObject:@"positionchange(bindEvent)"]
            lepusEventSet:nil
                    props:nil
                nodeIndex:0
       gestureDetectorSet:nil];
  LynxUI *ui = [owner findUIBySign:1];
  NSMapTable<LynxUI *, NSValue *> *rectCache = [owner valueForKey:@"lastPositionChangeRects"];
  [rectCache setObject:[NSValue valueWithCGRect:CGRectMake(1, 2, 3, 4)] forKey:ui];

  [owner recycleNode:1];

  XCTAssertNil([owner findUIBySign:1]);
  XCTAssertNil([rectCache objectForKey:ui]);
  XCTAssertEqual([[owner valueForKey:@"positionChangeListeners"] count], 0U);
  XCTAssertNil([owner valueForKey:@"positionChangeObserver"]);
  XCTAssertFalse([[owner valueForKey:@"positionChangeDispatchPending"] boolValue]);
}

@end
