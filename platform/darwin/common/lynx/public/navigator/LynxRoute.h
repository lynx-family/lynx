// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_LYNXROUTE_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_LYNXROUTE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxRoute : NSObject

@property NSString* templateUrl;
@property NSString* routeName;
@property NSDictionary* param;

- (instancetype)initWithUrl:(NSString*)url param:(NSDictionary*)param;

- (instancetype)initWithUrl:(NSString*)url
                  routeName:(NSString*)routeName
                      param:(NSDictionary*)param;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_LYNXROUTE_H_
