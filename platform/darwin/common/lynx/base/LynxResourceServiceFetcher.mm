// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxResourceServiceFetcher.h"

#import "LynxError.h"
#import "LynxLog.h"
#import "LynxResourceResponseDataInfoProtocol.h"
#import "LynxService.h"
#import "LynxServiceResourceProtocol.h"
#import "LynxSubErrorCode.h"

@implementation LynxResourceServiceFetcher

+ (id<LynxServiceResourceProtocol>)getLynxService {
  return LynxService(LynxServiceResourceProtocol);
}

+ (BOOL)ensureLynxService {
  return [LynxResourceServiceFetcher getLynxService] != nil;
}

// required interface, recommend to use loadResourceWithURLString directly
- (dispatch_block_t)loadResourceWithURL:(NSURL *)url
                                   type:(LynxFetchResType)type
                             completion:(LynxResourceLoadCompletionBlock)completionBlock {
  [self loadResourceWithURLString:[url relativePath] completion:completionBlock];
  return nil;
}

/*
 * send resource request with completion block
 * Notice: The isSyncCallback parameter of completionBlock does not truly reflect whether the
 * request is sync or not.
 * Do not rely on it !!!
 */
- (void)loadResourceWithURLString:(NSString *)urlString
                       completion:(LynxResourceLoadCompletionBlock)completionBlock {
  id<LynxServiceResourceProtocol> service = [LynxResourceServiceFetcher getLynxService];
  if (service != nil) {
    // Network requests should not be made on the main thread, so async method of forest are always
    // called
    [service fetchResourceAsync:urlString
                     parameters:[LynxServiceResourceRequestParameters new]
                     completion:^(id<LynxServiceResourceResponseProtocol> _Nullable response,
                                  NSError *_Nullable error) {
                       if (response == nil && error == nil) {
                         error = [LynxError
                             lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                   description:@"Lynx resource service response is nil"];
                       }
                       completionBlock(false, error ? nil : [response data], error, nil);
                     }];
  } else {
    completionBlock(true, nil,
                    [LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                     description:@"Lynx resource service init fail"],
                    nil);
  }
}

- (dispatch_block_t)requestAsyncWithResourceRequest:(LynxResourceRequest *)lynxResourceRequest
                                               type:(LynxFetchResType)type
                     lynxResourceLoadCompletedBlock:
                         (LynxResourceLoadCompletedBlock)loadCompletedBlock {
  if (lynxResourceRequest.url == nil) {
    loadCompletedBlock([[LynxResourceResponse alloc]
        initWithError:[LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                       description:@"Lynx resource request url is nil."]
                 code:LynxResourceResponseCodeFailed]);
    LLogError(@"LynxResourceServiceFetcher requestAsyncWithResourceRequest url is nil.");
    return nil;
  }
  id<LynxServiceResourceProtocol> service = [LynxResourceServiceFetcher getLynxService];
  if (service != nil) {
    LynxServiceResourceRequestParameters *parameters =
        [lynxResourceRequest getLynxResourceServiceRequestParams] != nil
            ? [lynxResourceRequest getLynxResourceServiceRequestParams]
            : [LynxServiceResourceRequestParameters new];
    id<LynxServiceResourceRequestOperationProtocol> operation = [service
        fetchResourceAsync:lynxResourceRequest.url
                parameters:parameters
                completion:^(id<LynxServiceResourceResponseProtocol> _Nullable response,
                             NSError *_Nullable error) {
                  if ((response == nil || ![response isSuccess]) && error == nil) {
                    error = [LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                             description:@"Lynx resource service request fail."];
                  }
                  if (error != nil) {
                    loadCompletedBlock([[LynxResourceResponse alloc]
                        initWithError:error
                                 code:LynxResourceResponseCodeFailed]);
                    LLogError(@"LynxResourceServiceFetcher requestAsyncWithResourceRequest error: "
                              @"%@, url:%@",
                              error.description, lynxResourceRequest.url);
                  } else {
                    loadCompletedBlock([[LynxResourceResponse alloc]
                        initWithData:(id<LynxResourceResponseDataInfoProtocol>)response]);
                    LLogInfo(@"LynxResourceServiceFetcher requestAsyncWithResourceRequest success, "
                             @"url:%@",
                             lynxResourceRequest.url);
                  }
                }];
    return ^() {
      [operation cancel];
    };
  } else {
    loadCompletedBlock([[LynxResourceResponse alloc]
        initWithError:[LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                       description:@"Lynx resource service init fail"]
                 code:LynxResourceResponseCodeFailed]);
    LLogError(@"LynxResourceServiceFetcher requestAsyncWithResourceRequest error: Lynx resource "
              @"service init fail, url:%@",
              lynxResourceRequest.url);
    return nil;
  }
}

- (nonnull LynxResourceResponse *)requestSyncWithResourceRequest:
                                      (LynxResourceRequest *)lynxResourceRequest
                                                            type:(LynxFetchResType)type {
  if (lynxResourceRequest.url == nil) {
    return [[LynxResourceResponse alloc]
        initWithError:[LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                       description:@"Lynx resource request url is nil."]
                 code:LynxResourceResponseCodeFailed];
    LLogError(@"LynxResourceServiceFetcher requestSyncWithResourceRequest url is nil.");
  }
  id<LynxServiceResourceProtocol> service = [LynxResourceServiceFetcher getLynxService];
  if (service != nil) {
    LynxServiceResourceRequestParameters *parameters =
        [lynxResourceRequest getLynxResourceServiceRequestParams] != nil
            ? [lynxResourceRequest getLynxResourceServiceRequestParams]
            : [LynxServiceResourceRequestParameters new];
    NSError *error = nil;
    id<LynxServiceResourceResponseProtocol> response =
        [service fetchResourceSync:lynxResourceRequest.url parameters:parameters error:&error];
    if (response && [response isSuccess]) {
      LLogInfo(@"LynxResourceServiceFetcher requestSyncWithResourceRequest success, url:%@",
               lynxResourceRequest.url);
      return [[LynxResourceResponse alloc]
          initWithData:(id<LynxResourceResponseDataInfoProtocol>)response];
    } else if (error) {
      LLogError(@"LynxResourceServiceFetcher requestSyncWithResourceRequest error: %@, url:%@",
                error.description, lynxResourceRequest.url);
      return [[LynxResourceResponse alloc] initWithError:error code:LynxResourceResponseCodeFailed];
    } else {
      LLogError(@"LynxResourceServiceFetcher requestSyncWithResourceRequest error: Lynx resource "
                @"service request fail, url:%@",
                lynxResourceRequest.url);
      return [[LynxResourceResponse alloc]
          initWithError:[LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                         description:@"Lynx resource service request fail."]
                   code:LynxResourceResponseCodeFailed];
    }
  } else {
    LLogError(@"LynxResourceServiceFetcher requestSyncWithResourceRequest error: Lynx resource "
              @"service init fail, url:%@",
              lynxResourceRequest.url);
    return [[LynxResourceResponse alloc]
        initWithError:[LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                       description:@"Lynx resource service init fail."]
                 code:LynxResourceResponseCodeFailed];
  }
}

@end
