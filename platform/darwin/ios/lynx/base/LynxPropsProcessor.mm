// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxPropsProcessor.h"
#import "LynxConverter.h"
#import "LynxLog.h"
#import "LynxShadowNode.h"
#import "LynxUI.h"

#import <objc/message.h>
#import <objc/runtime.h>

// Prop setter using for setting props to target
typedef void (^LynxPropSetter)(id target, id value);

// Store prop name and prop setter as key-value pair
// key : value => props name : setter
typedef NSMutableDictionary<NSString*, LynxPropSetter> PropSetterMap;

// A holder that hold all the prop setter function of registered component.
// key : value => class name : setter map
static NSMutableDictionary<NSString*, PropSetterMap*>* kLynxPropSetterHolder;

@implementation LynxPropsProcessor

/**
 * Preparing type selector for lately conversion by LynxConverter
 *
 * Example:
 *  NSInteger => toNSInteger:
 *  UIColor* => toUIColor:
 *
 */
+ (SEL)generateTypeSel:(NSString*)type {
  // Trims string
  type = [type stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  // Extracts pure type
  if ([type characterAtIndex:[type length] - 1] == '*') {
    type = [type substringToIndex:[type length] - 1];
    // Trims string again
    type = [type stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  }
  type = [@"to" stringByAppendingString:type];
  return NSSelectorFromString([type stringByAppendingString:@":"]);
}

/**
 * Wraps prop setter block with type conversion so that the prop value that
 * set to target can be the wanted one.
 */
+ (LynxPropSetter)wrapValueConversionWithType:(NSString*)type
                                    forSetter:(void (^)(id, void*, bool))setter {
  SEL typeSel = [self generateTypeSel:type];
  // Check if there are suitable conversion rule for type defined in LynxConverter
  NSMethodSignature* signature = [LynxConverter methodSignatureForSelector:typeSel];
  if (!signature) {
    return ^(id target, id value) {
      @try {
        setter(target, &value, (!value || [value isEqual:[NSNull null]]));
      } @catch (NSException* exception) {
        [self handlePropSetterException:exception with:target];
      }
    };
  } else {
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    invocation.selector = typeSel;
    invocation.target = [LynxConverter class];
    return ^(id target, id value) {
      // First convert value to wanted type
      bool reset = (!value || [value isEqual:[NSNull null]]) ? true : false;
      void* bakedValue = nil;
      bakedValue = malloc(signature.methodReturnLength);
      [invocation setArgument:&value atIndex:2];
      @try {
        [invocation invoke];
        [invocation getReturnValue:bakedValue];
        // Invoke method to set prop
        setter(target, bakedValue, reset);
      } @catch (NSException* exception) {
        [self handlePropSetterException:exception with:target];
      }
      free(bakedValue);
    };
  }
}

+ (void)handlePropSetterException:(NSException*)exception with:(id)target {
  if (target && [target isKindOfClass:[LynxUI class]]) {
    LynxUI* ui = (LynxUI*)target;
    NSString* message =
        [NSString stringWithFormat:@"Exception occurs while updating property in ui %@: %@: %@",
                                   ui.tagName, exception.name, exception.reason];
    [ui.context didReceiveException:exception withMessage:message forUI:ui];
  }
}

/**
 * Get all relevant methods that defined in class's method propSetterLookUp and
 * make them into prop setter accroding to the config.
 */
+ (PropSetterMap*)getSetterMapFromComponent:(Class)clazz
                       withPropSetterLookUp:(Method)methodPropSetter {
  NSString* type = nil;
  NSString* propName = nil;
  PropSetterMap* setterMap = [PropSetterMap new];
  SEL setterSel = nil;

  SEL selector = method_getName(methodPropSetter);
  IMP imp = [clazz methodForSelector:selector];
  NSArray<NSArray<NSString*>*>* arrays = ((NSArray<NSArray*> * (*)(id, SEL)) imp)(clazz, selector);
  for (id array in arrays) {
    if ([array count] == 2) {
      propName = array[0];
      setterSel = NSSelectorFromString(array[1]);
      Method method = class_getInstanceMethod(clazz, setterSel);
      char* argType = method_copyArgumentType(method, 0);
      if (argType) {
        type = [NSString stringWithUTF8String:argType];
        free(argType);
      }
      __block NSInvocation* targetInvocation;
      setterMap[propName] =
          [self wrapValueConversionWithType:type
                                  forSetter:^(id target, void* value, bool reset) {
                                    if (!targetInvocation) {
                                      NSMethodSignature* signature =
                                          [target methodSignatureForSelector:setterSel];
                                      targetInvocation =
                                          [NSInvocation invocationWithMethodSignature:signature];
                                      targetInvocation.selector = setterSel;
                                    }
                                    [targetInvocation setArgument:value atIndex:2];
                                    [targetInvocation setArgument:&reset atIndex:3];
                                    [targetInvocation invokeWithTarget:target];
                                  }];
    }
  }
  return setterMap;
}

LynxPropSetter LynxPropSetterFromMethodInfoArray(NSArray<NSString*>* methodInfo) {
  SEL setterSel = NSSelectorFromString([methodInfo[1] stringByAppendingString:@":requestReset:"]);
  NSString* type = methodInfo[2];
  __block NSInvocation* targetInvocation;
  LynxPropSetter result = [LynxPropsProcessor
      wrapValueConversionWithType:type
                        forSetter:^(id target, void* value, bool reset) {
                          if (!targetInvocation) {
                            NSMethodSignature* signature =
                                [target methodSignatureForSelector:setterSel];
                            targetInvocation =
                                [NSInvocation invocationWithMethodSignature:signature];
                            targetInvocation.selector = setterSel;
                          }
                          [targetInvocation setArgument:value atIndex:2];
                          [targetInvocation setArgument:&reset atIndex:3];
                          [targetInvocation invokeWithTarget:target];
                        }];
  return result;
}

/**
 * Get all relevant methods that defined in class through LYNX_PROP_SETTER and
 * make them into prop setter accroding to the prop config.
 */
+ (PropSetterMap*)getSetterMapFromComponent:(Class)clazz {
  PropSetterMap* result = [PropSetterMap new];
  unsigned int methodCount = 0;
  Method* methods = class_copyMethodList(object_getClass(clazz), &methodCount);
  for (unsigned int i = 0; i < methodCount; i++) {
    Method method = methods[i];
    SEL selector = method_getName(method);
    NSString* stringFromSelector = NSStringFromSelector(selector);
    if ([stringFromSelector hasPrefix:LYNX_PROPS_CONFIG_PREFIX_STR]) {
      // class method with prefix `__lynx_prop_config__` returns only one method info.
      IMP imp = method_getImplementation(method);
      NSArray<NSString*>* methodInfo = ((NSArray<NSString*> * (*)(id, SEL)) imp)(clazz, selector);
      result[methodInfo[0]] = LynxPropSetterFromMethodInfoArray(methodInfo);
    } else if ([stringFromSelector hasPrefix:LYNX_PROPS_GROUP_CONFIG_PREFIX_STR]) {
      // class method with prefix `__lynx_props_group_config__` returns an array of method info.
      IMP imp = method_getImplementation(method);
      NSArray<NSArray<NSString*>*>* arrays =
          ((NSArray<NSArray*> * (*)(id, SEL)) imp)(clazz, selector);
      for (NSArray<NSString*>* methodInfo in arrays) {
        if ([methodInfo count] == 3) {
          result[methodInfo[0]] = LynxPropSetterFromMethodInfoArray(methodInfo);
        }
      }
    }
  }
  free(methods);
  return result;
}

/**
 * Extracts all relevant methods that defined in class's method propSetterLookUp
 * or in class through LYNX_PROP_SETTER,  and make them into prop setter
 * accroding to the prop config.
 */
+ (void)extractPropSetterFromComponent:(Class)clazz
                              withName:(NSString*)clazzName
                  withPropSetterHolder:(NSMutableDictionary*)propSetterHolder {
  PropSetterMap* setterMap = [PropSetterMap new];

  Method methodPropSetter =
      class_getClassMethod(clazz, NSSelectorFromString(LYNX_PROPS_SETTER_LOOK_UP));
  if (methodPropSetter != nil) {
    setterMap = [self getSetterMapFromComponent:clazz withPropSetterLookUp:methodPropSetter];
  } else {
    setterMap = [self getSetterMapFromComponent:clazz];
  }
  if (!propSetterHolder[clazzName]) {
    [propSetterHolder setObject:setterMap forKey:clazzName];
  }
}

/**
 * In the main thread, SetterHolder is a single instance(kLynxPropSetterHolder).
 * In other threads, we manage SetterHolder by thread-local caches, to solve the problem of using
 * setter caches in multiple threads.
 */
+ (NSMutableDictionary*)getLynxPropsSetterHolder {
  // Initialize prop setter dictionary
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    kLynxPropSetterHolder = [NSMutableDictionary new];
  });
  if ([NSThread isMainThread]) {
    return kLynxPropSetterHolder;
  } else {
    NSMutableDictionary* threadLocalCache = [[NSThread currentThread] threadDictionary];
    NSMutableDictionary* propSetterHolder = threadLocalCache[@"lynx_prop_setter_holder"];
    if (propSetterHolder == nil) {
      propSetterHolder = [NSMutableDictionary new];
      threadLocalCache[@"lynx_prop_setter_holder"] = propSetterHolder;
    }
    return propSetterHolder;
  }
}

/**
 * Finds prop setter by clazz and prop name. Extract prop setter from class and it's
 * class if it hasn't been extracted.
 */
+ (LynxPropSetter)findPropSetterByComponent:(Class)clazz andKey:(NSString*)prop {
  NSMutableDictionary* propSetterHolder = [self getLynxPropsSetterHolder];

  LynxPropSetter setter = nil;
  do {
    NSString* clazzName = [[NSString alloc] initWithUTF8String:class_getName(clazz)];
    PropSetterMap* setterMap = propSetterHolder[clazzName];

    // If clazz has not been extracted, we should extract prop setter immediatly
    // and reassign setter map.
    if (!setterMap) {
      [self extractPropSetterFromComponent:clazz
                                  withName:clazzName
                      withPropSetterHolder:propSetterHolder];
      setterMap = propSetterHolder[clazzName];
    }

    setter = setterMap[prop];
    if (setter) {
      break;
    }

    // LynxUI and LynxShadowNode is basic class for component, it's not nessary to
    // extrat prop setter of their super class.
    if (clazz == [LynxUI class] || clazz == [LynxShadowNode class]) {
      break;
    }

    // Get super class for recursively extraction if not find setter in current class
    clazz = class_getSuperclass(clazz);

  } while (clazz != [NSObject class] && clazz != [NSProxy class]);
  return setter;
}

+ (void)updateProp:(id)value withKey:(NSString*)key forUI:(LynxUI*)ui {
  if (ui == nil) {
    return;
  }
  LynxPropSetter setter = [self findPropSetterByComponent:[ui class] andKey:key];
  if (setter) {
    setter(ui, value);
  }
}

+ (void)updateProp:(id)value withKey:(NSString*)key forShadowNode:(LynxShadowNode*)shadowNode {
  if (shadowNode == nil) {
    return;
  }
  LynxPropSetter setter = [self findPropSetterByComponent:[shadowNode class] andKey:key];
  if (setter) {
    setter(shadowNode, value);
  }
}

@end
