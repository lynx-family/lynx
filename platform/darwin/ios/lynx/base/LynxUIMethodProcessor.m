// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxUIMethodProcessor.h"
#import <objc/message.h>
#import <objc/runtime.h>
#import "LynxModule.h"
#import "LynxUI.h"

typedef void (^LynxUIMethod)(id target, NSDictionary* params, LynxUIMethodCallbackBlock callback);
typedef NSMutableDictionary<NSString*, LynxUIMethod> UIMethodMap;
static NSMutableDictionary<NSString*, UIMethodMap*>* kLynxUIMethodHolder;

@implementation LynxUIMethodProcessor

+ (void)invokeMethod:(NSString*)method
          withParams:(NSDictionary*)params
          withResult:(LynxUIMethodCallbackBlock)callback
               forUI:(LynxUI*)ui {
  LynxUIMethod operation = [self findUIOperationByComponent:[ui class] andKey:method];
  if (operation) {
    @try {
      operation(ui, params, callback);
    } @catch (NSException* exception) {
      NSString* message =
          [NSString stringWithFormat:@"Exception occurs while invoking method in ui %@: %@: %@",
                                     ui.tagName, exception.name, exception.reason];
      [ui.context didReceiveException:exception withMessage:message forUI:ui];
    }
  } else {
    if (callback) {
      NSString* id_selector = ui.idSelector;
      NSString* msg = [NSString stringWithFormat:@"method '%@' not found for ui %@", method,
                                                 [id_selector length] ? id_selector : @""];
      callback(kUIMethodMethodNotFound, msg);
    }
  }
}

+ (void)extractUIOperationFromComponent:(Class)clazz withName:clazzName {
  Method* methods = nil;
  UIMethodMap* methodMap = [UIMethodMap new];

  unsigned int methodCount;
  methods = class_copyMethodList(object_getClass(clazz), &methodCount);

  for (unsigned int i = 0; i < methodCount; i++) {
    Method method = methods[i];
    SEL selector = method_getName(method);
    if (![NSStringFromSelector(selector) hasPrefix:LYNX_UI_METHOD_CONFIG_PREFIX_STR]) {
      continue;
    }
    IMP imp = method_getImplementation(method);
    NSString* methodName = ((NSString * (*)(id, SEL)) imp)(clazz, selector);
    SEL operationSel = NSSelectorFromString([methodName stringByAppendingString:@":withResult:"]);
    __block NSInvocation* targetInvocation;

    methodMap[methodName] = ^(id target, NSDictionary* params, LynxUIMethodCallbackBlock callback) {
      if (!targetInvocation) {
        NSMethodSignature* signature = [target methodSignatureForSelector:operationSel];
        targetInvocation = [NSInvocation invocationWithMethodSignature:signature];
        targetInvocation.selector = operationSel;
      }

      [targetInvocation setArgument:&params atIndex:2];
      [targetInvocation setArgument:&callback atIndex:3];
      [targetInvocation invokeWithTarget:target];
    };
  }

  free(methods);

  kLynxUIMethodHolder[clazzName] = methodMap;
}

+ (LynxUIMethod)findUIOperationByComponent:(Class)clazz andKey:(NSString*)methodName {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    kLynxUIMethodHolder = [NSMutableDictionary new];
  });

  LynxUIMethod method = nil;
  do {
    NSString* clazzName = [[NSString alloc] initWithUTF8String:class_getName(clazz)];
    UIMethodMap* methodMap = kLynxUIMethodHolder[clazzName];

    if (!methodMap) {
      [self extractUIOperationFromComponent:clazz withName:clazzName];
      methodMap = kLynxUIMethodHolder[clazzName];
    }

    method = methodMap[methodName];

    if (method) {
      break;
    }

    if (clazz == [LynxUI class]) {
      break;
    }

    clazz = class_getSuperclass(clazz);

  } while (clazz != [NSObject class] && clazz != [NSProxy class]);

  return method;
}

@end
