// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@interface LynxKeyframes : NSObject

@property(nonatomic, strong) NSDictionary<NSString*, NSDictionary<NSString*, id>*>* styles;

@end
