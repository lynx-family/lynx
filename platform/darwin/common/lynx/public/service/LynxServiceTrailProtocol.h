// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETRAILPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETRAILPROTOCOL_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceProtocol;

@protocol LynxServiceTrailProtocol <LynxServiceProtocol>

/**
 * Get string value for key from experiment
 * @param key key of experiment
 */
- (NSString *)stringValueForTrailKey:(NSString *)key;

/**
 * Get object value for key from experiment. Only used for compatibility with different types,
 * please use stringValueForTrailKey in most cases
 * @param key key of experiment
 */
- (id)objectValueForTrailKey:(NSString *)key;

/**
 * Get all values for key from experiment.
 */
- (NSDictionary *)getAllValues;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETRAILPROTOCOL_H_
