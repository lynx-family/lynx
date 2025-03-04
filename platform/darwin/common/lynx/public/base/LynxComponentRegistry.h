// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#if __has_include("LynxLazyLoad.h")
#import "LynxLazyLoad.h"
#ifndef LYNX_LAZY_LOAD
#define LYNX_LAZY_LOAD 1
#endif
#endif
/**
 * Register ui class when app starts which will be used by LynxUIOwner.
 *
 * the order of registration is not exact. It's possible that the
 * previous ui will be replace by the current class with the same name.
 *
 * @param name, the tag name used for displaying in front-end
 */
#define LYNX_REGISTER_UI(name)                              \
  +(void)load {                                             \
    [LynxComponentRegistry registerUI:self withName:@name]; \
  }

/**
 * Register shadow node class when app starts which will be used by LynxShadowNodeOwner.
 *
 * the order of registration is not exact. It's possible that the
 * previous ui will be replace by the current class with the same name.
 *
 * @param name, the tag name used for displaying in front-end
 */
#define LYNX_REGISTER_SHADOW_NODE(name)                             \
  +(void)load {                                                     \
    [LynxComponentRegistry registerShadowNode:self withName:@name]; \
  }

/**
 * Registry for shadow node and ui which are called component.
 */

@interface LynxComponentRegistry : NSObject

+ (void)registerUI:(Class)componentClass nameAs:(NSString*)name;
+ (void)registerNode:(Class)componentClass nameAs:(NSString*)name;
+ (void)registerUI:(Class)componentClass withName:(NSString*)name;
+ (void)registerShadowNode:(Class)componentClass withName:(NSString*)name;
+ (Class)shadowNodeClassWithName:(NSString*)name accessible:(BOOL*)legal;
+ (Class)uiClassWithName:(NSString*)name accessible:(BOOL*)legal;
+ (NSSet<NSString*>*)lynxUIClasses;
@end

@interface LynxComponentScopeRegistry : NSObject

@property(nonatomic, readonly, assign) NSSet<NSString*>* allRegisteredComponent;

- (void)registerUI:(Class)componentClass withName:(NSString*)name;
- (void)registerShadowNode:(Class)componentClass withName:(NSString*)name;
- (Class)shadowNodeClassWithName:(NSString*)name accessible:(BOOL*)legal;
- (Class)uiClassWithName:(NSString*)name accessible:(BOOL*)legal;

- (void)makeIntoGloabl;

@end
