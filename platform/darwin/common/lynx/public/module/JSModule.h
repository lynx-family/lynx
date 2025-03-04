// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_MODULE_JSMODULE_H_
#define DARWIN_COMMON_LYNX_MODULE_JSMODULE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JSModule : NSObject

@property(nonatomic, nullable) NSString* moduleName;

- (instancetype)initWithModuleName:(nonnull NSString*)moduleName;

- (void)fire:(nonnull NSString*)methodName withParams:(NSArray*)args;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_MODULE_JSMODULE_H_
