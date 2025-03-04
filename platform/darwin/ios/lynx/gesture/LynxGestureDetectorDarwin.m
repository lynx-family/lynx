// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGestureDetectorDarwin.h>

@implementation LynxGestureDetectorDarwin

- (instancetype)initWithGestureID:(uint32_t)gestureID
                      gestureType:(LynxGestureTypeDarwin)gestureType
             gestureCallbackNames:(NSArray<NSString *> *)gestureCallbackNames
                      relationMap:(NSDictionary<NSString *, NSArray<NSNumber *> *> *)relationMap {
  self = [super init];
  if (self) {
    // Initialize gesture properties.
    _gestureID = gestureID;
    _gestureType = gestureType;
    _gestureCallbackNames = [gestureCallbackNames copy];
    _relationMap = [relationMap copy];
  }
  return self;
}

- (instancetype)initWithGestureID:(uint32_t)gestureID
                      gestureType:(LynxGestureTypeDarwin)gestureType
             gestureCallbackNames:(NSArray<NSString *> *)gestureCallbackNames
                      relationMap:(NSDictionary<NSString *, NSArray<NSNumber *> *> *)relationMap
                        configMap:(NSMutableDictionary *)configMap {
  self = [super init];
  if (self) {
    // Initialize gesture properties.
    _gestureID = gestureID;
    _gestureType = gestureType;
    _gestureCallbackNames = [gestureCallbackNames copy];
    _relationMap = [relationMap copy];
    _configMap = [configMap copy];
  }
  return self;
}

@end
