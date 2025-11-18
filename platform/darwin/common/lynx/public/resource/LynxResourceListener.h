// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCELISTENER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCELISTENER_H_

#import <Foundation/Foundation.h>
#import <Lynx/LynxResourceRequest.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxResourceLoadSuccessInfo : NSObject

@property(nonatomic, copy, readonly) NSString *url;

- (instancetype)initWithURL:(NSString *)url;

@end

@interface LynxResourceLoadFailedInfo : NSObject

@property(nonatomic, copy, readonly) NSString *errorMessage;
@property(nonatomic, readonly) NSInteger errorCode;
@property(nonatomic, readonly) NSInteger categorizedCode;

- (instancetype)initWithErrorMessage:(NSString *)errorMessage
                           errorCode:(NSInteger)errorCode
                     categorizedCode:(NSInteger)categorizedCode;

@end

@protocol LynxResourceListener <NSObject>

/**
 * Called immediately before a resource request is submitted
 * @param url The resource URL being requested
 * @param type The category of resource being requested
 */
@optional
- (void)onResourceLoadStart:(NSString *)url type:(LynxResourceRequestType)type;

/**
 * Called when a resource request completes successfully
 * @param url The requested resource URL
 * @param type The category of the loaded resource
 * @param reportInfo Contains detailed information about the successful load resource
 */
@optional
- (void)onResourceLoadSuccess:(NSString *)url
                         type:(LynxResourceRequestType)type
                   reportInfo:(LynxResourceLoadSuccessInfo *)reportInfo;

/**
 * Called when a resource request fails to complete
 * @param url The requested resource URL
 * @param type The category of the failed resource
 * @param errorInfo Contains failure details including error code and message
 */
@optional
- (void)onResourceLoadFailed:(NSString *)url
                        type:(LynxResourceRequestType)type
                   errorInfo:(LynxResourceLoadFailedInfo *)errorInfo;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCELISTENER_H_
