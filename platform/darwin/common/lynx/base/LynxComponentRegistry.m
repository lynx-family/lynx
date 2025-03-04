// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxComponentRegistry.h"
#import <objc/message.h>
#import <objc/runtime.h>
#import "LynxThreadSafeDictionary.h"
#if OS_IOS
#import "LynxPropsProcessor.h"
#import "LynxShadowNode.h"
#import "LynxUI.h"
#endif

@implementation LynxComponentRegistry

static LynxThreadSafeDictionary<NSString*, Class>* LynxUIClasses;
static LynxThreadSafeDictionary<NSString*, Class>* LynxShadowNodeClasses;

+ (void)registerUI:(Class)componentClass nameAs:(NSString*)name {
#if OS_IOS
  Method methodPropSetter =
      class_getClassMethod(componentClass, NSSelectorFromString(LYNX_PROPS_SETTER_LOOK_UP));
  if ([componentClass isSubclassOfClass:LynxUI.class] && methodPropSetter != nil) {
    [LynxComponentRegistry registerUI:componentClass withName:name];
  }
#endif
}

+ (void)registerNode:(Class)componentClass nameAs:(NSString*)name {
#if OS_IOS
  Method methodPropSetter =
      class_getClassMethod(componentClass, NSSelectorFromString(LYNX_PROPS_SETTER_LOOK_UP));
  if ([componentClass isSubclassOfClass:LynxShadowNode.class] && methodPropSetter != nil) {
    [LynxComponentRegistry registerShadowNode:componentClass withName:name];
  }
#endif
}

+ (void)registerUI:(Class)componentClass withName:(NSString*)name {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    LynxUIClasses = [LynxThreadSafeDictionary new];
  });
  if (componentClass && name) {
    [LynxUIClasses setObject:componentClass forKey:name];
  }
}

+ (void)registerShadowNode:(Class)componentClass withName:(NSString*)name {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    LynxShadowNodeClasses = [LynxThreadSafeDictionary new];
  });
  if (componentClass && name) {
    [LynxShadowNodeClasses setObject:componentClass forKey:name];
  }
}

+ (Class)shadowNodeClassWithName:(NSString*)name accessible:(BOOL*)legal {
  Class res = LynxShadowNodeClasses[name];
  if (res != nil || [self supportNode:name]) {
    *legal = YES;
  } else {
    *legal = NO;
  }
  return LynxShadowNodeClasses[name];
}

+ (Class)uiClassWithName:(NSString*)name accessible:(BOOL*)legal {
  Class res = LynxUIClasses[name];
  if (res != nil || [self supportUI:name]) {
    *legal = YES;
  } else {
    *legal = NO;
  }
  return LynxUIClasses[name];
}

+ (BOOL)supportUI:(NSString*)name {
  return [name isEqualToString:@"Root"] || [name isEqualToString:@"view"] ||
         LynxShadowNodeClasses[name] != nil;
}

+ (BOOL)supportNode:(NSString*)name {
  return [name isEqualToString:@"Root"] || [name isEqualToString:@"view"] ||
         LynxUIClasses[name] != nil;
}

+ (NSSet<NSString*>*)lynxUIClasses {
  return [[NSSet<NSString*> alloc] initWithArray:[LynxUIClasses allKeys]];
}

@end

@interface LynxComponentScopeRegistry ()

@property(nonatomic) NSMutableDictionary<NSString*, Class>* uiClasses;
@property(nonatomic) NSMutableDictionary<NSString*, Class>* shadowNodeClasses;

@end

@implementation LynxComponentScopeRegistry

- (instancetype)init {
  self = [super init];
  if (self) {
    _uiClasses = [NSMutableDictionary new];
    _shadowNodeClasses = [NSMutableDictionary new];
  }
  return self;
}

- (void)registerUI:(Class)componentClass withName:(NSString*)name {
#if OS_IOS
  if ([componentClass isSubclassOfClass:LynxUI.class]) {
    [_uiClasses setObject:componentClass forKey:name];
  }
#endif
}

- (void)registerShadowNode:(Class)componentClass withName:(NSString*)name {
#if OS_IOS
  if ([componentClass isSubclassOfClass:LynxShadowNode.class]) {
    [_shadowNodeClasses setObject:componentClass forKey:name];
  }
#endif
}

- (Class)shadowNodeClassWithName:(NSString*)name accessible:(BOOL*)legal {
  Class res = _shadowNodeClasses[name];
  if (res != nil || _uiClasses[name]) {
    *legal = YES;
  } else {
    res = [LynxComponentRegistry shadowNodeClassWithName:name accessible:legal];
  }
  return res;
}

- (Class)uiClassWithName:(NSString*)name accessible:(BOOL*)legal {
  Class res = _uiClasses[name];
  if (res != nil) {
    *legal = YES;
  } else {
    res = [LynxComponentRegistry uiClassWithName:name accessible:legal];
  }
  return res;
}

- (void)makeIntoGloabl {
  for (NSString* name in _uiClasses) {
    [LynxComponentRegistry registerUI:[_uiClasses valueForKey:name] withName:name];
  }
  for (NSString* name in _shadowNodeClasses) {
    [LynxComponentRegistry registerShadowNode:[_shadowNodeClasses valueForKey:name] withName:name];
  }
}

- (NSSet<NSString*>*)allRegisteredComponent {
  NSMutableSet<NSString*>* component =
      [[NSMutableSet<NSString*> alloc] initWithArray:[_uiClasses allKeys]];
  return [component setByAddingObjectsFromSet:[LynxComponentRegistry lynxUIClasses]];
}

@end
