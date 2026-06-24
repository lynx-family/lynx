// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxResourceFetcher.h>
#import <Lynx/LynxView+Internal.h>
#import <XElement/LynxAnimaXResourceFetcherLoader.h>
#include "src/base/log/log.h"

@interface LynxAnimaXResourceFetcherLoader ()
@property(nonatomic, assign) BOOL useLegacyFetcher;
@property(nonatomic, weak, nullable) id<LynxGenericResourceFetcher> genericFetcher;
@property(nonatomic, weak, nullable) id<LynxResourceFetcher> resourceFetcher;

- (instancetype)initWithGenericFetcher:(nullable id<LynxGenericResourceFetcher>)genericFetcher
                       resourceFetcher:(nullable id<LynxResourceFetcher>)resourceFetcher
    NS_DESIGNATED_INITIALIZER;
@end

@implementation LynxAnimaXResourceFetcherLoader

+ (BOOL)isLegacyFetcherEnabled {
  static BOOL isLegacyFetcherEnabled = NO;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    isLegacyFetcherEnabled =
        [LynxEnv stringValueToBool:[[LynxEnv sharedInstance]
                                       _stringFromExternalEnv:@"ANIMAX_USE_LEGACY_FETCHER"]
                      defaultValue:NO];
    if (isLegacyFetcherEnabled) {
      ANIMAX_LOGI("Generic fetcher unavailable: setting is disabled");
    }
  });
  return isLegacyFetcherEnabled;
}

- (instancetype)initWithGenericFetcher:(nullable id<LynxGenericResourceFetcher>)genericFetcher
                       resourceFetcher:(nullable id<LynxResourceFetcher>)resourceFetcher {
  self = [super init];
  if (self) {
    _genericFetcher = genericFetcher;
    _resourceFetcher = resourceFetcher;
    _useLegacyFetcher = [LynxAnimaXResourceFetcherLoader isLegacyFetcherEnabled];
  }
  if (!_genericFetcher) {
    ANIMAX_LOGI("Generic fetcher unavailable: fetcher is null");
  }
  return self;
}

+ (LynxAnimaXResourceFetcherLoader*)loaderWithLynxUIContext:(LynxUIContext*)context {
  // Try to get legacy fetcher
  id<LynxResourceFetcher> resourceFetcher = context.resourceFetcher;
  if (!resourceFetcher) {
    ANIMAX_LOGI("context.resourceFetcher is null, trying LynxView fallback");
    LynxView* lynxView = [context.lynxContext getLynxView];
    if (lynxView) {
      resourceFetcher = lynxView.resourceFetcher;
    } else {
      ANIMAX_LOGI("Legacy fetcher unavailable: lynxView is null");
    }
  }

  return
      [[LynxAnimaXResourceFetcherLoader alloc] initWithGenericFetcher:context.genericResourceFetcher
                                                      resourceFetcher:resourceFetcher];
}

+ (LynxAnimaXResourceFetcherLoader*)loaderWithLynxGenericFetcher:
    (nullable id<LynxGenericResourceFetcher>)genericFetcher {
  return [[LynxAnimaXResourceFetcherLoader alloc] initWithGenericFetcher:genericFetcher
                                                         resourceFetcher:nil];
}

+ (LynxAnimaXResourceFetcherLoader*)loaderWithLynxResourceFetcher:
    (nullable id<LynxResourceFetcher>)resourceFetcher {
  return [[LynxAnimaXResourceFetcherLoader alloc] initWithGenericFetcher:nil
                                                         resourceFetcher:resourceFetcher];
}

- (void)fetchURL:(NSURL*)request_url
     withFetcher:(id<LynxResourceFetcher>)fetcher
      completion:(AnimaXLoaderCompletionHandler)completion {
  [fetcher
      loadResourceWithURL:request_url
                     type:LynxFetchResUnknown
               completion:^(BOOL isSyncCallback, NSData* _Nullable data, NSError* _Nullable error,
                            NSURL* _Nullable resURL) {
                 AnimaXLoaderResponse* response = nil;

                 if (error) {
                   response = [AnimaXLoaderResponse responseWithError:error];

                 } else if (data && [data length] > 0) {
                   response = [AnimaXLoaderResponse responseWithPayload:data];
                 } else if (resURL) {
                   response = [AnimaXLoaderResponse responseWithPayload:[resURL path]];
                 } else {
                   response = [AnimaXLoaderResponse
                       responseWithErrorMessage:@"Response from LynxResourceFetcher is neither a "
                                                @"valid NSData or a valid file path NSURL"];
                 }
                 completion(response);
               }];
}

- (void)handleRequest:(AnimaXLoaderRequest*)request
           completion:(AnimaXLoaderCompletionHandler)completion {
  NSString* url = request.url;
  AnimaXLoaderResponse* response = nil;
  do {
    if (nil == request || nil == request.url || [request.url length] == 0) {
      response = [AnimaXLoaderResponse responseWithErrorMessage:@"Invalid AnimaXLoaderRequest."];
      break;
    }

    NSURL* request_url = [NSURL URLWithString:url];
    if (!request_url) {
      response = [AnimaXLoaderResponse responseWithErrorMessage:@"Could not convert URL to NSURL."];
      break;
    }

    // Try generic fetcher first
    if ([self tryGenericFetcher:request_url completion:completion]) {
      return;
    }

    // Fallback to legacy fetcher
    if ([self tryLegacyFetcher:request_url completion:completion]) {
      return;
    }

    response = [AnimaXLoaderResponse responseWithErrorMessage:@"No available resource fetcher."];
  } while (0);
  completion(response);
}

- (BOOL)tryGenericFetcher:(NSURL*)requestUrl completion:(AnimaXLoaderCompletionHandler)completion {
  // Check if generic fetcher is enabled from environment
  if (self.useLegacyFetcher) {
    return NO;
  }

  if (!self.genericFetcher) {
    return NO;
  }

  LynxResourceRequest* request = [[LynxResourceRequest alloc] initWithUrl:requestUrl.absoluteString
                                                                     type:LynxResourceTypeLottie];

  [self.genericFetcher
      fetchResource:request
         onComplete:^(NSData* _Nullable data, NSError* _Nullable error) {
           if (error) {
             completion([AnimaXLoaderResponse responseWithError:error]);
           } else if (data) {
             completion([AnimaXLoaderResponse responseWithPayload:data]);
           } else {
             completion([AnimaXLoaderResponse responseWithErrorMessage:@"No data received"]);
           }
         }];

  return YES;
}

- (BOOL)tryLegacyFetcher:(NSURL*)requestUrl completion:(AnimaXLoaderCompletionHandler)completion {
  if (!self.resourceFetcher) {
    ANIMAX_LOGI("Legacy fetcher unavailable: fetcher is null");
    return NO;
  }

  [self fetchURL:requestUrl withFetcher:self.resourceFetcher completion:completion];
  return YES;
}

- (AnimaXLoaderScheme)getScheme {
  return AnimaXLoaderSchemeHttp;
}
@end
