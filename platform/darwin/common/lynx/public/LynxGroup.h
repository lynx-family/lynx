// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXGROUP_H_
#define DARWIN_COMMON_LYNX_LYNXGROUP_H_

#import <Foundation/Foundation.h>

@class LynxView;

@interface LynxGroupOption : NSObject

@property(nonatomic, readwrite, nullable) NSArray *preloadJSPaths;
@property(nonatomic, readwrite) bool enableJSGroupThread;

- (void)setStringConfig:(nonnull NSString *)value forKey:(nonnull NSString *)key;
- (void)setBoolConfig:(BOOL)value forKey:(nonnull NSString *)key;

@end

/**
 * A class used to distinguish between different LynxViews.
 */
@interface LynxGroup : NSObject

/*!
 The name of LynxGroup
 */
@property(nonatomic, readonly, nonnull) NSString *groupName;

/*!
 The ID of LynxGroup
 */
@property(nonatomic, readonly, nonnull) NSString *identification;
@property(nonatomic, readonly, nullable) NSArray *preloadJSPaths;

/**
 * The return value of the function is the tag of the LynxView which doesn't belong to any group.
 */
+ (nonnull NSString *)singleGroupTag;

/**
 * Init LynxGroup with name.
 */
- (nonnull instancetype)initWithName:(nonnull NSString *)name;

/**
 * Init LynxGroup with name and extra js scripts path.
 */
- (nonnull instancetype)initWithName:(nonnull NSString *)name
                   withPreloadScript:(nullable NSArray *)extraJSPaths;

/**
 * Init LynxGroup with name and the option of group.
 */
- (nonnull instancetype)initWithName:(nonnull NSString *)name
                 withLynxGroupOption:(nullable LynxGroup *)option;

/**
 * Add LynxView to this group.
 */
- (void)addLynxView:(nonnull LynxView *)view;

- (bool)enableJSGroupThread;

- (nullable NSString *)getStringConfig:(nonnull NSString *)key;
- (BOOL)getBoolConfig:(nonnull NSString *)key;

@end

#endif  // DARWIN_COMMON_LYNX_LYNXGROUP_H_
