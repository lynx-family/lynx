// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXGENERICRESOURCEFETCHER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXGENERICRESOURCEFETCHER_H_

#import <Lynx/LynxResourceRequest.h>
#import <Lynx/LynxResourceResponse.h>

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

/**
 * @apidoc
 * @brief `LynxGenericResourceFetcher` is defined inside `LynxEngine`
 * and injected from outside to implement a general resource loading interface.
 * It is used inside LynxEngine for resource loading capabilities of components such as `Text`
 */
@protocol LynxGenericResourceFetcher <NSObject>

/**
 * @apidoc
 * @brief `LynxEngine` internally calls this method to obtain the general
 * resource content, and the return result is required to be the resource content `NSData*` type.
 *
 * @param request Request for the requiring resource.
 * @param callback Contents of the requiring resource.
 * @note This method must be implemented.
 */
@required
- (dispatch_block_t)fetchResource:(nonnull LynxResourceRequest *)request
                       onComplete:(LynxGenericResourceCompletionBlock _Nonnull)callback;

/**
 * @apidoc
 * @brief `LynxEngine` internally calls this method to obtain the path of the common
 * resource on the local disk, and the return result is required to be of `String` type.
 *
 * @param request Request for the requiring resource.
 * @param callback Path on the disk of the requiring resource.
 * @note This method must be implemented.
 */
@required
- (dispatch_block_t)fetchResourcePath:(nonnull LynxResourceRequest *)request
                           onComplete:(LynxGenericResourcePathCompletionBlock _Nonnull)callback;

/**
 * @apidoc
 * @brief `LynxEngine` internally calls this method to acquire the bytecode content of the
 * input request, and the return result is required to be the resource content `NSData*` type.
 *
 * @param request Request for the requiring resource.
 * @param callback Contents of the requiring resource.
 * @note This method is optional to be implemented.
 *
 * @Since 3.6
 */
@optional
- (dispatch_block_t)fetchBytecode:(nonnull LynxResourceRequest *)request
                       onComplete:(LynxGenericResourceCompletionBlock _Nonnull)callback;

/**
 * @apidoc
 * @brief `LynxEngine` internally calls this method to obtain resource content in a streaming
 * manner.
 *
 * @param request Request for the requiring resource.
 * @param delegate Streaming of the requiring resource.
 * @note This method is optional to be implemented.
 */
@optional
- (dispatch_block_t)fetchStream:(nonnull LynxResourceRequest *)request
                     withStream:(nonnull id<LynxResourceStreamLoadDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXGENERICRESOURCEFETCHER_H_
