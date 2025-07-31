// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_FLUENCY_LYNXFLUENCYMONITOR_H_
#define PLATFORM_DARWIN_IOS_LYNX_FLUENCY_LYNXFLUENCYMONITOR_H_

#import <Lynx/LynxBooleanOption.h>
#import <Lynx/LynxScrollListener.h>

NS_ASSUME_NONNULL_BEGIN

/// @brief The configuration object used to identify the monitoring instance.
@interface LynxFluencyConfig : NSObject

/// @brief The key used to identify the monitoring instance.
@property(nonatomic, nonnull, strong) id<NSCopying> key;
/// @brief The tag name used to identify the monitoring instance.
@property(nonatomic, nullable, copy) NSString *tagName;
/// @brief The tag name used to identify the scroll monitoring instance if it is existing.
@property(nonatomic, nullable, copy) NSString *scrollMonitorTagName;
/// @brief The instance id of the LynxView.
@property(nonatomic, assign) int32_t instanceId;

/// @brief Initialize the configuration object with the specified key, tag name, scroll monitor tag
/// name and instance id.
/// @param key The key used to identify the monitoring instance.
/// @param tagName The tag name used to identify the monitoring instance.
/// @param scrollMonitorTagName The tag name used to identify the scroll monitoring instance if it
/// is existing.
/// @param instanceId The instance id of the LynxView.
- (instancetype)initWithKey:(id<NSCopying>)key
                    tagName:(NSString *)tagName
       scrollMonitorTagName:(NSString *)scrollMonitorTagName
                 instanceId:(int32_t)instanceId;

/// @brief Initialize the configuration object with the specified key, tag name, scroll monitor tag
/// name and instance id.
/// @param key The key used to identify the monitoring instance.
/// @param tagName The tag name used to identify the monitoring instance.
/// @param instanceId The instance id of the LynxView.
- (instancetype)initWithKey:(id<NSCopying>)key
                    tagName:(NSString *)tagName
                 instanceId:(int32_t)instanceId;

@end

@interface LynxFluencyMonitor : NSObject

@property(nonatomic, readonly) BOOL shouldSendAllScrollEvent;

/// @breif Start the fluency monitoring with the specified configuration.
/// @param config The fluency configuration object used to identify the monitoring instance to be
/// start.
- (void)startWithFluencyConfig:(LynxFluencyConfig *)config;

/// @breif Stop the fluency monitoring with the specified configuration.
/// @param config The fluency configuration object used to identify the monitoring instance to be
/// stopped.
- (void)stopWithFluencyConfig:(LynxFluencyConfig *)config;

/// Set the sampling decision of whether to enable fluency metics collection.
///
/// If PageConfig is not configured with kEnableLynxScrollFluency, Lynx will use
/// the value passed to `LynxView(Builder).setFluencyTracerEnabled:` to determine
/// whether to enable fluency metics collection.
///
/// @note This method is only effective when PageConfig is not configured with
/// kEnableLynxScrollFluency. See `setPageConfigProbability:` for more details.
///
/// @param enabledBySampling The sampling decision of whether to enable fluency
/// metics collection.
- (void)setEnabledBySampling:(LynxBooleanOption)enabledBySampling;

/// Set the probability of enabling fluency metics collection based on PageConfig.
///
/// If PageConfig is configured with kEnableLynxScrollFluency, Lynx will determine
/// whether to enable fluency metics based on this probability when creating a
/// LynxView.
- (void)setPageConfigProbability:(CGFloat)probability;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_FLUENCY_LYNXFLUENCYMONITOR_H_
