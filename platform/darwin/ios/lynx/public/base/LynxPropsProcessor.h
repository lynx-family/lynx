// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxDefines.h"
#import "LynxPreprocessingUtils.h"

/**
 * A special prefix for method name
 */
#define LYNX_PROPS_CONFIG_PREFIX __lynx_prop_config__
#define LYNX_PROPS_CONFIG_PREFIX_STR @"__lynx_prop_config__"

#define LYNX_PROPS_GROUP_CONFIG_PREFIX __lynx_props_group_config__
#define LYNX_PROPS_GROUP_CONFIG_PREFIX_STR @"__lynx_props_group_config__"

#define LYNX_PROPS_SETTER_LOOK_UP @"propSetterLookUp"

/**
 * Generate an unique method name for prop config
 */
#define LYNX_PROP_CONFIG_METHOD \
  LYNX_CONCAT(LYNX_PROPS_CONFIG_PREFIX, LYNX_CONCAT(__LINE__, __COUNTER__))

#define LYNX_PROPS_GROUP_CONFIG_METHOD \
  LYNX_CONCAT(LYNX_PROPS_GROUP_CONFIG_PREFIX, LYNX_CONCAT(__LINE__, __COUNTER__))

/**
 * Use this macro to define props setter method.
 *
 * Two relevant method will be generated and the setter must be implement.
 * The prop config method will be used for collecting setter info.
 *
 * @param name, the prop name
 * @param method, the setter name
 * @param type, the prop value type
 */
#define LYNX_PROP_SETTER(name, method, type)                   \
  +(NSArray<NSString*>*)LYNX_PROP_CONFIG_METHOD LYNX_DYNAMIC { \
    return @[ @name, @ #method, @ #type ];                     \
  }                                                            \
  -(void)method : (type)value requestReset : (BOOL)requestReset LYNX_DYNAMIC

#define LYNX_PROP_INFO_ARRAY(name, method, type) @[ @name, @ #method, @ #type ],

/**
 * Use the result of this macro as parameters of `LYNX_PROPS_GROUP_DECLARE` to declare a prop setter
 * For example: LYNX_PROPS_GROUP_DECLARE(LYNX_PROP_DECLARE("scale", setScale, CGFloat))
 * @param name, the prop name
 * @param method, the setter name
 * @param type, the prop value type
 */
#define LYNX_PROP_DECLARE(name, method, type) (name, method, type)

/**
 * Use this macro to declare a group of prop setters.
 * The parameter for this macro must be the result of `LYNX_PROP_DECLARE`
 * This macro generates an class method like below:
 *
 * +(NSArray<NSArray<NSString*>*>*)__lynx_props_group_config__340 {
 *   return @[ @[ @"scroll-event-throttle", @ "setScrollEventThrottle", @ "CGFloat" ],
 *          @[ @"upper-threshold", @ "setUpperThreshold", @ "CGFloat" ] ];
 * };
 *
 * the class method generated will return an array of methodInfos.
 * methodInfo[0] is the name of the prop.
 * `methodInfo[1]:requestReset:` is the selector for the prop, for example:
 * `setScrollEventThrottle:requestReset:` methodInfo[2] is the type, as string, of the first
 * parameter of the selector
 *
 * @warning this macro supports no more than 128 `LYNX_PROP_DECLARE`.
 * Make another group if the number of `LYNX_PROP_DECLARE` exceeds 128. Or generate a new
 * `LynxPreprocessingUtil.h`
 */
#define LYNX_PROPS_GROUP_DECLARE(...)                              \
  +(NSArray<NSArray<NSString*>*>*)LYNX_PROPS_GROUP_CONFIG_METHOD { \
    return @[ LYNX_MACRO_MAP(LYNX_PROP_INFO_ARRAY, __VA_ARGS__) ]; \
  }

/**
 * Use this macro to define props setter method.
 *
 * This macro will generate an instance method `-(void)method : (type)value requestReset :
 * (BOOL)requestReset`. You have to implement the detail of the method by appending the method body
 * This macro should be used together with `LYNX_PROP_DECLARE` and `LYNX_PROPS_GROUP_DECLARE`
 *
 * @param name, the prop name
 * @param method, the setter name
 * @param type, the prop value type
 */
#define LYNX_PROP_DEFINE(name, method, type) \
  -(void)method : (type)value requestReset : (BOOL)requestReset

@class LynxUI;
@class LynxShadowNode;

/**
 * A processor to help shadow node and ui to set props through the function
 * that defines by the macro LYNX_PROP_SETTER.
 */
@interface LynxPropsProcessor : NSObject

+ (void)updateProp:(id)value withKey:(NSString*)key forUI:(LynxUI*)ui;

+ (void)updateProp:(id)value withKey:(NSString*)key forShadowNode:(LynxShadowNode*)shadowNode;

@end
