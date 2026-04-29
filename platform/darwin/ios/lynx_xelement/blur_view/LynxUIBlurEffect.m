// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIBlurEffect.h>
#import <objc/runtime.h>

@interface LynxUIBlurEffect ()
@property(nonatomic, assign) CGFloat blurRadius;
@end

@implementation LynxUIBlurEffect
+ (UIBlurEffect* _Nullable)effectWithStyle:(UIBlurEffectStyle)style blurRadius:(CGFloat)radius {
  UIBlurEffect* effect = [self effectWithStyle:style];
  if ([effect isKindOfClass:self]) {
    LynxUIBlurEffect* blurEffect = (LynxUIBlurEffect*)effect;
    blurEffect.blurRadius = radius;
  }
  return effect;
}

+ (void)initialize {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [self customBlurConfig];
  });
}

+ (void)customBlurConfig {
  // UIBlurEffect get blur settings via method '- (id)effectSettings'. The
  // return value contains configs for the effect.
  SEL effectSettings = NSSelectorFromString(@"effectSettings");
  if (![self instancesRespondToSelector:effectSettings]) {
    return;
  }

  __block id (*func)(id, SEL) = NULL;
  // clang-format off
  // Following is the customized implementation of method 'effectSettings'.
  // 1. create the config instance via original implementation.
  // 2. iterate ivar list and modify the '_blurRadius' field if exists.
  // 3. return the objects.
  // clang-format on
  IMP imp = imp_implementationWithBlock(^id(UIBlurEffect* effect) {
    id config = nil;
    // call the original effectSettings.
    if (func) {
      config = func(effect, effectSettings);
    }

    if ([effect isKindOfClass:LynxUIBlurEffect.class]) {
      LynxUIBlurEffect* blurEffect = (LynxUIBlurEffect*)effect;
      Class cls = [config class];
      while (cls && cls != [NSObject class]) {
        unsigned int uVarCount = 0;
        Ivar* pVarList = class_copyIvarList(cls, &uVarCount);
        for (unsigned int i = 0; i < uVarCount; ++i) {
          Ivar pVar = pVarList[i];
          const char* varName = ivar_getName(pVar);
          NSString* strVarName = [NSString stringWithUTF8String:varName];
          double* configIvarPtr = (double*)((uint8_t*)(long)config + ivar_getOffset(pVar));
          if ([strVarName isEqualToString:@"_blurRadius"] && blurEffect.blurRadius > 0) {
            *configIvarPtr = blurEffect.blurRadius;
          }
        }
        if (pVarList) {
          free(pVarList);
          pVarList = NULL;
        }
        // find instance variables held by superclass
        cls = class_getSuperclass(cls);
      }
    }
    return config;
  });

  Method method = class_getInstanceMethod(self, effectSettings);
  // Save the original implementation of 'effectSettings'.
  func = (id(*)(id, SEL))method_getImplementation(method);
  // Set customized implementation to enable blur radius modification.
  class_replaceMethod(self, effectSettings, imp, method_getTypeEncoding(method));
}
@end
