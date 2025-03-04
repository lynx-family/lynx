// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Enum for representing different types of gestures
typedef NS_ENUM(NSUInteger, LynxGestureTypeDarwin) {
  LynxGestureTypePan = 0,
  LynxGestureTypeFling = 1,
  LynxGestureTypeDefault = 2,
  LynxGestureTypeTap = 3,
  LynxGestureTypeLongPress = 4,
  LynxGestureTypeRotation = 5,
  LynxGestureTypePinch = 6,
  LynxGestureTypeNative = 7
};

@interface LynxGestureDetectorDarwin : NSObject

- (instancetype)initWithGestureID:(uint32_t)gestureID
                      gestureType:(LynxGestureTypeDarwin)gestureType
             gestureCallbackNames:(NSArray<NSString *> *)gestureCallbackNames
                      relationMap:(NSDictionary<NSString *, NSArray<NSNumber *> *> *)relationMap;

- (instancetype)initWithGestureID:(uint32_t)gestureID
                      gestureType:(LynxGestureTypeDarwin)gestureType
             gestureCallbackNames:(NSArray<NSString *> *)gestureCallbackNames
                      relationMap:(NSDictionary<NSString *, NSArray<NSNumber *> *> *)relationMap
                        configMap:(NSMutableDictionary *)configMap;

@property(nonatomic, assign, readonly) uint32_t gestureID;
@property(nonatomic, assign, readonly) LynxGestureTypeDarwin gestureType;
@property(nonatomic, copy, readonly) NSArray<NSString *> *gestureCallbackNames;
@property(nonatomic, copy, readonly) NSDictionary<NSString *, NSArray<NSNumber *> *> *relationMap;
@property(nonatomic, copy, readonly) NSMutableDictionary *configMap;

@end

NS_ASSUME_NONNULL_END
