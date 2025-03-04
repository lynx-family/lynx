// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxLayoutTick.h"
#import "LynxShadowNode.h"
#import "LynxUIOwner.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxShadowNodeType) {
  LynxShadowNodeTypeCommon = 1,
  LynxShadowNodeTypeVirtual = 1 << 1,
  LynxShadowNodeTypeCustom = 1 << 2,
  //  LynxShadowNodeTypeFlatten = 1 << 3,
  LynxShadowNodeTypeInlineView = 1 << 5,
};

@class LynxComponentRegistry;

@interface LynxShadowNodeOwner : NSObject

@property(atomic, readonly, nullable) LynxLayoutTick* layoutTick;
@property(nonatomic, weak, readonly) LynxUIContext* uiContext;
@property(nonatomic, assign) void* layoutNodeManagerPtr;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithUIOwner:(LynxUIOwner*)uiOwner
                     layoutTick:(nullable LynxLayoutTick*)layoutTick
                  isAsyncLayout:(BOOL)isAsyncLayout;

- (void)setDelegate:(id<LynxShadowNodeDelegate>)delegate;

- (void)setLayoutNodeManager:(void*)layoutNodeManager;

- (NSInteger)createNodeWithSign:(NSInteger)sign
                        tagName:(nonnull NSString*)tagName
                          props:(nullable NSDictionary*)props
                       eventSet:(nullable NSSet<NSString*>*)eventSet
                  lepusEventSet:(nullable NSSet<NSString*>*)lepusEventSet
        isParentInlineContainer:(bool)allowInline;

- (void)updateNodeWithSign:(NSInteger)sign
                     props:(nullable NSDictionary*)props
                  eventSet:(nullable NSSet<NSString*>*)eventSet
             lepusEventSet:(nullable NSSet<NSString*>*)lepusEventSet;

- (void)insertNode:(NSInteger)childSign toParent:(NSInteger)parentSign atIndex:(NSInteger)index;

- (void)removeNode:(NSInteger)childSign fromParent:(NSInteger)parentSign atIndex:(NSInteger)index;

- (void)moveNode:(NSInteger)childSign
        inParent:(NSInteger)parentSign
       fromIndex:(NSInteger)from
         toIndex:(NSInteger)to;

- (void)destroyNode:(NSInteger)sign;

- (void)didLayoutStartOnNode:(NSInteger)sign;

- (void)didUpdateLayoutLeft:(CGFloat)left
                        top:(CGFloat)top
                      width:(CGFloat)width
                     height:(CGFloat)height
                     onNode:(NSInteger)sign;

- (void)didLayoutFinished;

- (void)destroy;
- (void)destroySelf;

- (LynxShadowNode*)nodeWithSign:(NSInteger)sign;

- (void)updateRootSize:(float)width height:(float)height;
- (float)rootWidth;
- (float)rootHeight;
@end

NS_ASSUME_NONNULL_END
