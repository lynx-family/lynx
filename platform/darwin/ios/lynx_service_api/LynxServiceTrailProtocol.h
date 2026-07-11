// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_SERVICE_API_LYNXSERVICETRAILPROTOCOL_H_
#define DARWIN_SERVICE_API_LYNXSERVICETRAILPROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxServiceAPI/ServiceAPI.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxViewBuilder;

@protocol LynxServiceProtocol;

typedef void (^LynxTrailFetchCallback)(BOOL success, NSString *_Nullable errorMessage);

/** Immutable values and metadata for one host-defined Trail layer. */
@interface LynxTrailValueLayer : NSObject

@property(nonatomic, copy, readonly) NSString *name;
@property(nonatomic, assign, readonly) NSInteger updatedAt;
@property(nonatomic, copy, readonly) NSDictionary<NSString *, NSString *> *values;

- (instancetype)initWithName:(NSString *)name
                   updatedAt:(NSInteger)updatedAt
                      values:(NSDictionary<NSString *, NSString *> *)values;

@end

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

@optional

/**
 * Returns an immutable snapshot of the value layers maintained by this service.
 *
 * A layer is a host-defined source of Trail values. Implementations may provide any number of
 * layers with non-empty, unique names. The returned list is ordered from highest to lowest
 * priority: when the same key exists in multiple layers, the value in the first matching layer is
 * effective. The name `mock` is reserved for local mock overrides. When present, the mock layer
 * must be the first layer in the list.
 *
 * Each layer contains all values discoverable by the implementation. A layer's `updatedAt` is
 * implementation-defined; zero means that its update time is unknown or unsupported.
 */
- (NSArray<LynxTrailValueLayer *> *)getLayeredValues;

/** Sets a process-wide mock override. */
- (BOOL)setMockValue:(NSString *)value forKey:(NSString *)key;

/** Removes a process-wide mock override. */
- (BOOL)removeMockValueForKey:(NSString *)key;

/** Clears all process-wide mock overrides. */
- (BOOL)clearMockValues;

/** Fetches the latest values from the implementation-defined backing source. */
- (void)fetchLatestSettings:(LynxTrailFetchCallback)callback;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_SERVICE_API_LYNXSERVICETRAILPROTOCOL_H_
