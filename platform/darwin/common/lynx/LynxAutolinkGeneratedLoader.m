// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxConfig.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxLazyLoad.h>
#import <Lynx/LynxTemplateProvider.h>
#import <objc/runtime.h>

static char kLynxAutolinkGeneratedConfigSetupKey;

static Class LynxAutolinkGeneratedRegistryClass(void) {
  Class registryClass = objc_getClass("LynxGeneratedExtensionRegistry");
  SEL setupSelector = NSSelectorFromString(@"setup:");
  if (registryClass == Nil || class_getInstanceMethod(registryClass, setupSelector) == NULL) {
    return Nil;
  }
  return registryClass;
}

static void LynxAutolinkSetupConfig(LynxConfig *config) {
  Class registryClass = LynxAutolinkGeneratedRegistryClass();
  if (registryClass == Nil || config == nil) {
    return;
  }
  @synchronized(config) {
    if ([objc_getAssociatedObject(config, &kLynxAutolinkGeneratedConfigSetupKey) boolValue]) {
      return;
    }
    objc_setAssociatedObject(config, &kLynxAutolinkGeneratedConfigSetupKey, @YES,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }

  id registry = [[registryClass alloc] init];
  SEL setupSelector = NSSelectorFromString(@"setup:");
  typedef void (*LynxAutolinkSetupImp)(id, SEL, LynxConfig *);
  LynxAutolinkSetupImp setupImp = (LynxAutolinkSetupImp)[registry methodForSelector:setupSelector];
  if (setupImp != NULL) {
    setupImp(registry, setupSelector, config);
  }
}

static void LynxAutolinkSwizzleInstanceMethod(Class targetClass, SEL originalSelector,
                                              SEL swizzledSelector) {
  Method originalMethod = class_getInstanceMethod(targetClass, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(targetClass, swizzledSelector);
  if (originalMethod == NULL || swizzledMethod == NULL) {
    return;
  }
  method_exchangeImplementations(originalMethod, swizzledMethod);
}

@interface LynxConfig (LynxAutolinkGenerated)
- (instancetype)lynx_autolink_initWithProvider:(id<LynxTemplateProvider>)provider;
@end

@implementation LynxConfig (LynxAutolinkGenerated)
- (instancetype)lynx_autolink_initWithProvider:(id<LynxTemplateProvider>)provider {
  LynxConfig *config = [self lynx_autolink_initWithProvider:provider];
  LynxAutolinkSetupConfig(config);
  return config;
}
@end

@interface LynxEnv (LynxAutolinkGenerated)
- (void)lynx_autolink_prepareConfig:(LynxConfig *)config;
@end

@implementation LynxEnv (LynxAutolinkGenerated)
- (void)lynx_autolink_prepareConfig:(LynxConfig *)config {
  LynxAutolinkSetupConfig(config);
  [self lynx_autolink_prepareConfig:config];
}
@end

@interface LynxAutolinkGeneratedLoader : NSObject
@end

@implementation LynxAutolinkGeneratedLoader

+ (void)lynxLazyLoad {
  LYNX_BASE_INIT_METHOD
  if (LynxAutolinkGeneratedRegistryClass() == Nil) {
    return;
  }
  LynxAutolinkSwizzleInstanceMethod([LynxConfig class], @selector(initWithProvider:),
                                    @selector(lynx_autolink_initWithProvider:));
  LynxAutolinkSwizzleInstanceMethod([LynxEnv class], @selector(prepareConfig:),
                                    @selector(lynx_autolink_prepareConfig:));
}

@end
