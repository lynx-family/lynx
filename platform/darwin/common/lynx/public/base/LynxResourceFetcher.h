// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXRESOURCEFETCHER_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXRESOURCEFETCHER_H_

#import <Foundation/Foundation.h>
#import "LynxResourceRequest.h"
#import "LynxResourceResponse.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxFetchResType) {
  LynxFetchResUnknown = 0,
  LynxFetchResFontFace,
  LynxFetchResImage,
  LynxFetchResLottie,
  LynxFetchResVideo,
  LynxFetchResSVG,
  LynxFetchResTemplate,
  LynxFetchResLynxCoreJS,
  LynxFetchResDynamicComponent,
  LynxFetchResI18NText,
  LynxFetchResTheme,
  LynxFetchResExternalJSSource,
  LynxFetchResURLOnlineOrOffline,
  LynxFetchResURLOnline,
  LynxFetchResURLOffline,
  LynxFetchResURLUnzipped,
  LynxFetchResData
};

@class LynxTheme;
@class LynxView;

typedef void (^LynxResourceLoadCompletionBlock)(BOOL isSyncCallback, NSData *_Nullable data,
                                                NSError *_Nullable error, NSURL *_Nullable resURL);
typedef void (^LynxResourceResolveHandler)(NSString *resolvedURL, id fetcher, id params,
                                           NSError *_Nullable error);
typedef void (^LynxResourceCompletionHandler)(NSData *_Nullable data, NSError *_Nullable error);
typedef void (^LynxLocalFileCompletionHandler)(NSURL *_Nullable url, NSError *_Nullable error);
typedef void (^LynxResourceLoadCompletedBlock)(LynxResourceResponse *_Nonnull response);

@protocol LynxResourceLoadDelegate <NSObject>
@required
- (void)onStart:(NSInteger)contentLength;  // length or -1
- (void)onData:(NSData *)data;
- (void)onEnd;
- (void)onError:(NSString *)msg;
@end

@protocol LynxResourceFetcher <NSObject>

// use this method first for SJB, add extra res types to help
// deal with resource callback
/*!
 Add extra res types to help
 Load resource asynchronously.
 @param url: target resource url.
 @param type: target resource type.
 @param completionBlock: the block to provide resource fetcher result.
 @return: A block which can cancel the resource request if it is not finished. nil if cancel action
 is not supported.
*/
@required
- (dispatch_block_t)loadResourceWithURL:(NSURL *)url
                                   type:(LynxFetchResType)type
                             completion:(LynxResourceLoadCompletionBlock)completionBlock;

/*!
 Add extra res types to help
 Load resource asynchronously.
 Lynx will not try to download resource if completion is failed
 @param url: target resource url.
 @param type: target resource type.
 @param completionBlock: the block to provide resource fetcher result.
 @return: A block which can cancel the resource request if it is not finished. nil if cancel action
 is not supported.
*/
@optional
- (dispatch_block_t)fetchResourceWithURL:(NSURL *)url
                                    type:(LynxFetchResType)type
                              completion:(LynxResourceLoadCompletionBlock)completionBlock;

/*!
 Load themed resource string
 @param resId: string for resource id
 @param theme: theme
 @param key: if not nil, only this key in theme will be match
 @param view: view
 @return: result resource string
*/
@optional
- (NSString *)translatedResourceWithId:(NSString *)resId
                                 theme:(LynxTheme *)theme
                              themeKey:(NSString *)key
                                  view:(__weak LynxView *)view;

/*!
 Redirect url
 @param urlString: string for url
 @return: result
*/
@optional
- (NSString *)redirectURL:(NSString *)urlString;

// switch to this later to take over all resource fetch
/*!
 Resolve resource origin url to url for different environment
 @param url: string for current source url
 @param completionBlock: the block to provide resolved resource url and customized resource loader
 like forest
 */
@optional
- (void)resolveResourceURL:(NSString *)url completion:(LynxResourceResolveHandler)completionBlock;

/*!
 Provide extra request info to container
 @param lynxModuleExtraData extra request data
 */
@optional
- (void)storeExtraModuleData:(id)lynxModuleExtraData;

@optional
- (void)fetchResourceDataWithURLString:(NSString *)urlString
                               context:(nullable NSDictionary *)context
                     completionHandler:(LynxResourceCompletionHandler)completionHandler;

@optional
- (void)fetchLocalFileWithURLString:(NSString *)urlString
                            context:(nullable NSDictionary *)context
                  completionHandler:(LynxLocalFileCompletionHandler)completionHandler;

@optional
- (dispatch_block_t)loadResourceWithURL:(NSURL *)url
                               delegate:(nonnull id<LynxResourceLoadDelegate>)delegate;

/*!
 load Resource with completion block
 @param urlString: target resource url.
 @param completionBlock: the block to provide resource fetcher result.
*/
@optional
- (void)loadResourceWithURLString:(NSString *)urlString
                       completion:(LynxResourceLoadCompletionBlock)completionBlock;

/*!
 Request resource asynchronously with load block.
 @param lynxResourceRequest: contains target resource url and request  params.
 @param type: target resource type.
 @param loadBlock: the block to provide resource fetcher response.
*/
@optional
- (_Nullable dispatch_block_t)
    requestAsyncWithResourceRequest:(LynxResourceRequest *)lynxResourceRequest
                               type:(LynxFetchResType)type
     lynxResourceLoadCompletedBlock:(LynxResourceLoadCompletedBlock)loadCompletedBlock;

/*!
 Request resource synchronously.
 @param lynxResourceRequest: contains target resource url and request  params.
 @param type: target resource type.
 @return LynxResourceResponse contains data and error.
*/
@optional
- (nonnull LynxResourceResponse *)requestSyncWithResourceRequest:
                                      (LynxResourceRequest *)lynxResourceRequest
                                                            type:(LynxFetchResType)type;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXRESOURCEFETCHER_H_
