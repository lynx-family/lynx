// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXGENERICRESOURCEFETCHER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXGENERICRESOURCEFETCHER_H_

#import "LynxResourceRequest.h"
#import "LynxResourceResponse.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxGenericResourceCompletionBlock)(NSData *_Nullable data,
                                                   NSError *_Nullable error);
typedef void (^LynxGenericResourcePathCompletionBlock)(NSString *_Nullable path,
                                                       NSError *_Nullable error);

@protocol LynxResourceStreamLoadDelegate <NSObject>
@required
/// Load process started
/// @param contentLength total length or -1 means unknown
- (void)onStart:(NSInteger)contentLength;

// Load process return part of data
/// @param data
/// May be called once or more
- (void)onData:(nullable NSData *)data;

/// Load process ended
- (void)onEnd;

/// Load ended with error
- (void)onError:(nullable NSString *)msg;

@end

@protocol LynxGenericResourceFetcher <NSObject>

/**
 * fetch resource with contents.
 *
 * @param request.
 * @param callback contents of the requiring resource.
 *
 * @return: A block which can cancel the image request if it is not finished. nil if cancel action
 * is not supported.
 */
@required
- (dispatch_block_t)fetchResource:(nonnull LynxResourceRequest *)request
                       onComplete:(LynxGenericResourceCompletionBlock _Nonnull)callback;

/**
 * fetch resource with res path.
 *
 * @param request
 * @param callback path on the disk of the requiring resource.
 *
 * @return: A block which can cancel the image request if it is not finished. nil if cancel action
 * is not supported.
 */
@required
- (dispatch_block_t)fetchResourcePath:(nonnull LynxResourceRequest *)request
                           onComplete:(LynxGenericResourcePathCompletionBlock _Nonnull)callback;

/**
 * fetch resource with stream.
 *
 * @param request
 * @param delegate streaming of the requiring resource.
 *
 * @return: A block which can cancel the image request if it is not finished. nil if cancel action
 * is not supported.
 */
@optional
- (dispatch_block_t)fetchStream:(nonnull LynxResourceRequest *)request
                     withStream:(nonnull id<LynxResourceStreamLoadDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXGENERICRESOURCEFETCHER_H_
