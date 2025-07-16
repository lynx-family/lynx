// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * @brief The LynxTimingCollectorProtocol defines the interface for timing collection.
 * This protocol is adopted by classes that need to collect and manage timing data.
 */
@protocol LynxTimingCollectorProtocol <NSObject>

@required
/**
 * @brief Marks a timing event with the specified key and pipeline ID.
 * @param key The key that uniquely identifies the timing event.
 * @param pipelineID The optional pipeline ID associated with the timing event.
 */
- (void)markTiming:(NSString *)key pipelineID:(nullable NSString *)pipelineID;

/**
 * @brief Sets a timing value with the specified timestamp, key, and pipeline ID.
 * @param timestamp The timestamp of the timing event.
 * @param key The key that uniquely identifies the timing event.
 * @param pipelineID The optional pipeline ID associated with the timing event.
 */
- (void)setTiming:(uint64_t)timestamp
              key:(NSString *)key
       pipelineID:(nullable NSString *)pipelineID;

/**
 * @brief Resets timing data before a reload operation.
 *
 * This method should be called when the lynx application is reloading to clear any previously
 * collected timing data.
 */
- (void)resetTimingBeforeReload;

/**
 * @brief Notifies the timing collector about the start of a pipeline.
 *
 * This method should be called when a new pipeline is started to record the timing data.
 *
 * @param pipelineID The unique identifier of the started pipeline.
 * @param pipelineOrigin The origin of the pipeline, such as "lynx", "webview", etc.
 * @param timestamp The timestamp of the pipeline start event.
 */
- (void)onPipelineStart:(NSString *)pipelineID
         pipelineOrigin:(NSString *)pipelineOrigin
              timestamp:(uint64_t)timestamp;

@end

NS_ASSUME_NONNULL_END
