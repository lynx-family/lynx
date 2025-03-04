// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxShadowNodeOwner.h"
#import "LynxComponentRegistry.h"
#import "LynxDefines.h"
#import "LynxEnv.h"
#import "LynxEventReporter.h"
#import "LynxKeyframeAnimator.h"
#import "LynxNativeLayoutNode.h"
#import "LynxPropsProcessor.h"
#import "LynxService.h"
#import "LynxThreadSafeDictionary.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"

@implementation LynxShadowNodeOwner {
  __weak LynxUIOwner* _uiOwner;
  NSMutableDictionary<NSNumber*, LynxShadowNode*>* _nodeHolder;
  // Record tags that use shadownode.
  NSMutableSet<NSString*>* _tagSet;
  LynxComponentScopeRegistry* _componentRegistry;
  BOOL _destroyed;
  id<LynxShadowNodeDelegate> _delegate;
  float _rootWidth;
  float _rootHeight;
}

LYNX_NOT_IMPLEMENTED(-(instancetype)init)

- (instancetype)initWithUIOwner:(LynxUIOwner*)uiOwner
                     layoutTick:(LynxLayoutTick*)layoutTick
                  isAsyncLayout:(BOOL)isAsyncLayout {
  self = [super init];
  if (self) {
    _uiOwner = uiOwner;
    _layoutTick = layoutTick;
    _uiContext = uiOwner.uiContext;
    if (isAsyncLayout) {
      _nodeHolder = [[LynxThreadSafeDictionary alloc] init];
    } else {
      _nodeHolder = [[NSMutableDictionary alloc] init];
    }
    _tagSet = [[NSMutableSet alloc] init];
    _componentRegistry = [uiOwner getComponentRegistry];
  }
  if (_uiOwner != nil && _uiOwner.uiContext != nil) {
    _uiOwner.uiContext.nodeOwner = self;
  }
  return self;
}

- (void)setDelegate:(id<LynxShadowNodeDelegate>)delegate {
  _delegate = delegate;
}

- (void)setLayoutNodeManager:(void*)layoutNodeManagerPtr {
  _layoutNodeManagerPtr = layoutNodeManagerPtr;
}

- (NSInteger)createNodeWithSign:(NSInteger)sign
                        tagName:(nonnull NSString*)tagName
                          props:(nullable NSDictionary*)props
                       eventSet:(nullable NSSet<NSString*>*)eventSet
                  lepusEventSet:(nullable NSSet<NSString*>*)lepusEventSet
        isParentInlineContainer:(bool)allowInline {
  BOOL supported = YES;
  Class clazz = [_componentRegistry shadowNodeClassWithName:tagName accessible:&supported];
  if (supported) {
    LynxShadowNode* node;
    NSInteger type = 0;
    if (clazz) {
      node = [[clazz alloc] initWithSign:sign tagName:tagName];
      type |= LynxShadowNodeTypeCustom;
    } else {
      type |= LynxShadowNodeTypeCommon;
      if (allowInline) {
        node = [[LynxNativeLayoutNode alloc] initWithSign:sign tagName:tagName];
      } else {
        return type;
      }
    }
    [node setUIOperation:_uiOwner];
    [node setDelegate:_delegate];
    [node setLayoutNodeManagerPtr:_layoutNodeManagerPtr];
    [_nodeHolder setObject:node forKey:[NSNumber numberWithInteger:sign]];
    // update props for shadow node
    for (NSString* key in props) {
      [LynxPropsProcessor updateProp:props[key] withKey:key forShadowNode:node];
    }
    if (!_destroyed) {
      int64_t deprecatedLayoutNodePtr = 1L;
      [node adoptNativeLayoutNode:deprecatedLayoutNodePtr];
    }
    if ([node isVirtual]) {
      type |= LynxShadowNodeTypeVirtual;
    }
    if (allowInline && ([node supportInlineView])) {
      type |= LynxShadowNodeTypeInlineView;
    }
    if ([node needsEventSet] && (eventSet != nil || lepusEventSet != nil)) {
      node.eventSet = [LynxEventSpec convertRawEvents:eventSet andRwaLepusEvents:lepusEventSet];
    }
    return type;
  } else {
    @throw [NSException
        exceptionWithName:@"LynxCreateNodeException"
                   reason:[NSString stringWithFormat:@"%@ node not found when create Node", tagName]
                 userInfo:nil];
  }
}

- (void)updateNodeWithSign:(NSInteger)sign
                     props:(nullable NSDictionary*)props
                  eventSet:(nullable NSSet<NSString*>*)eventSet
             lepusEventSet:(nullable NSSet<NSString*>*)lepusEventSet {
  LynxShadowNode* node = _nodeHolder[[NSNumber numberWithInteger:sign]];
  NSAssert(node, @"Can not find shadow node for sign:%ld", (long)sign);

  // update props for shadow node
  for (NSString* key in props) {
    [LynxPropsProcessor updateProp:props[key] withKey:key forShadowNode:node];
  }

  if ([node needsEventSet] && (eventSet != nil || lepusEventSet != nil)) {
    node.eventSet = [LynxEventSpec convertRawEvents:eventSet andRwaLepusEvents:lepusEventSet];
  }
}

- (void)insertNode:(NSInteger)childSign toParent:(NSInteger)parentSign atIndex:(NSInteger)index {
  LynxShadowNode* childNode = _nodeHolder[[NSNumber numberWithInteger:childSign]];
  LynxShadowNode* parentNode = _nodeHolder[[NSNumber numberWithInteger:parentSign]];
  NSAssert(childNode, @"Can not find child shadow node for sign:%ld", (long)childSign);
  NSAssert(parentNode, @"Can not find parent shadow node for sign:%ld", (long)parentSign);
  if (index == -1) {
    index = [[parentNode children] count];
  }
  [parentNode insertChild:childNode atIndex:index];
}

- (void)removeNode:(NSInteger)childSign fromParent:(NSInteger)parentSign atIndex:(NSInteger)index {
  LynxShadowNode* childNode = _nodeHolder[[NSNumber numberWithInteger:childSign]];
  LynxShadowNode* parentNode = _nodeHolder[[NSNumber numberWithInteger:parentSign]];
  NSAssert(childNode, @"Can not find child shadow node for sign:%ld", (long)childSign);
  NSAssert(parentNode, @"Can not find parent shadow node for sign:%ld", (long)parentSign);
  [parentNode removeChild:childNode atIndex:index];
}

- (void)moveNode:(NSInteger)childSign
        inParent:(NSInteger)parentSign
       fromIndex:(NSInteger)from
         toIndex:(NSInteger)to {
  LynxShadowNode* childNode = _nodeHolder[[NSNumber numberWithInteger:childSign]];
  LynxShadowNode* parentNode = _nodeHolder[[NSNumber numberWithInteger:parentSign]];
  NSAssert(childNode, @"Can not find child shadow node for sign:%ld", (long)childSign);
  NSAssert(parentNode, @"Can not find parent shadow node for sign:%ld", (long)parentSign);
  [parentNode removeChild:childNode atIndex:from];
  [parentNode insertChild:childNode atIndex:to];
}

- (void)destroyNode:(NSInteger)sign {
  LynxShadowNode* node = _nodeHolder[[NSNumber numberWithInteger:sign]];
  if (!node) {
    NSAssert(node, @"Can not find shadow node for sign:%ld", (long)sign);
    return;
  }
  [node destroy];
  [_nodeHolder removeObjectForKey:[NSNumber numberWithInteger:sign]];
}

- (void)didLayoutStartRecursivelyOnNode:(LynxShadowNode*)node {
  if ([node needsLayout]) {
    [node layoutDidStart];
    for (LynxShadowNode* child in node.children) {
      [self didLayoutStartRecursivelyOnNode:child];
    }
  }
}

- (void)didLayoutStartOnNode:(NSInteger)sign {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxShadowNodeOwner.didLayoutStartOnNode");
  LynxShadowNode* node = _nodeHolder[[NSNumber numberWithInteger:sign]];
  [node layoutDidStart];
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)didUpdateLayoutLeft:(CGFloat)left
                        top:(CGFloat)top
                      width:(CGFloat)width
                     height:(CGFloat)height
                     onNode:(NSInteger)sign {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxShadowNodeOwner.didUpdateLayout");
  LynxShadowNode* node = _nodeHolder[[NSNumber numberWithInteger:sign]];
  [node updateLayoutWithFrame:CGRectMake(left, top, width, height)];
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)didLayoutFinished {
  [_delegate finishLayoutOperation];
}

- (void)destroy {
  for (NSNumber* sign in _nodeHolder.allKeys) {
    LynxShadowNode* node = _nodeHolder[sign];
    [node destroy];
  }
  [_nodeHolder removeAllObjects];
}

- (void)destroySelf {
  _destroyed = YES;
}

- (LynxShadowNode*)nodeWithSign:(NSInteger)sign {
  if (_nodeHolder == nil) {
    return nil;
  }
  return [_nodeHolder objectForKey:@(sign)];
}

- (void)updateRootSize:(float)width height:(float)height {
  _rootWidth = width;
  _rootHeight = height;
}

- (float)rootWidth {
  return _rootWidth;
}

- (float)rootHeight {
  return _rootHeight;
}

@end
