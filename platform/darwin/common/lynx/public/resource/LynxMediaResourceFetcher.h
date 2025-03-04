// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXMEDIARESOURCEFETCHER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXMEDIARESOURCEFETCHER_H_

#import <UIKit/UIKit.h>
#import "LynxResourceRequest.h"
#import "LynxResourceResponse.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxMediaResourceCompletionBlock)(UIImage* _Nullable uiImage,
                                                 NSError* _Nullable error);

typedef NS_ENUM(NSInteger, LynxResourceOptionalBool) {
  LynxResourceOptionalBoolTrue,
  LynxResourceOptionalBoolFalse,
  LynxResourceOptionalBoolUndefined
};

@protocol LynxMediaResourceFetcher <NSObject>

@required
- (NSString* _Nonnull)shouldRedirectUrl:(LynxResourceRequest* _Nonnull)request;

// TODO(zhoupeng.z): rename to isLocalResource
/**
 * Quick check for a local path.
 *
 * @param url input path
 * @return TRUE if is a local path; FALSE if not a local path
 */
@optional
- (LynxResourceOptionalBool)isLocalResource:(NSURL* _Nonnull)url;

/**
 * fetch UIImage directly.
 *
 * @param request
 * @param callback Response with the needed uiImage.
 *
 * @return A block which can cancel the image request if it is not finished. nil if cancel action is
 * not supported.
 */
@optional
- (dispatch_block_t)fetchUIImage:(LynxResourceRequest* _Nonnull)request
                      onComplete:(LynxMediaResourceCompletionBlock _Nonnull)response;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXMEDIARESOURCEFETCHER_H_
