// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DemoTemplateResourceFetcher.h"
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceDevToolProtocol.h>
#import <LynxDevtool/LynxRecorderEnv.h>

@interface DemoTemplateResourceFetcher ()

- (BOOL)shouldSendAsyncRequest:(NSURL *)url;

@end

@implementation DemoTemplateResourceFetcher

// scheme: file://lynx?local://
+ (LocalBundleResult)readLocalBundleFromResource:(NSString *)url {
  LocalBundleResult res = {NO, NO, nil, nil, nil};
  NSString *encodeUrl =
      [url stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet
                                                                  URLFragmentAllowedCharacterSet]];
  NSURL *source = [NSURL URLWithString:encodeUrl];
  if ([source.scheme isEqualToString:@"file"]) {
    NSURL *subSourceUrl = [NSURL URLWithString:source.query];
    if ([subSourceUrl.scheme isEqualToString:@"local"]) {
      res.isLocalScheme = YES;
      res.query = [subSourceUrl query];
      NSString *localUrl =
          [NSString stringWithFormat:@"Resource/%@%@", subSourceUrl.host, subSourceUrl.path];
      NSString *extension = localUrl.pathExtension;
      res.url = [[NSBundle mainBundle] pathForResource:[localUrl stringByDeletingPathExtension]
                                                ofType:extension];
      if (res.url == nil) {
        Class inspectorOwnerClass = [LynxService(LynxServiceDevToolProtocol) inspectorOwnerClass];
        NSBundle *devtoolFrameworkBundle = [NSBundle bundleForClass:inspectorOwnerClass];
        NSURL *debugBundleUrl = [devtoolFrameworkBundle URLForResource:@"LynxDebugResources"
                                                         withExtension:@"bundle"];
        NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
        localUrl = [NSString stringWithFormat:@"%@%@", subSourceUrl.host, subSourceUrl.path];
        res.url = [bundle pathForResource:[localUrl stringByDeletingPathExtension]
                                   ofType:extension];
      }
      res.data = [NSData dataWithContentsOfFile:res.url];
    } else if ([url hasPrefix:[LynxRecorderEnv sharedInstance].lynxRecorderUrlPrefix]) {
      res.isLynxRecorderSchema = YES;
      res.url = url;
    }
  }
  return res;
}

- (BOOL)shouldSendAsyncRequest:(NSURL *)url {
  NSURLComponents *components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:NO];
  // Local resource URLs carry their query inside the nested local:// URL.
  if ([components.scheme isEqualToString:@"file"]) {
    NSURLComponents *local = [NSURLComponents componentsWithString:components.query];
    if ([local.scheme isEqualToString:@"local"]) {
      components = local;
    }
  }
  for (NSURLQueryItem *item in components.queryItems) {
    if ([item.name isEqualToString:@"sync"]) {
      return ![item.value isEqualToString:@"true"];
    }
  }
  return YES;
}

- (void)fetchTemplate:(LynxResourceRequest *)request
           onComplete:(LynxTemplateResourceCompletionBlock)callback {
  NSURLComponents *components = [NSURLComponents componentsWithString:request.url];
  BOOL sync = ![self shouldSendAsyncRequest:[NSURL URLWithString:request.url]];
  void (^completeLocal)(LynxTemplateResource *, NSError *) =
      ^(LynxTemplateResource *resource, NSError *error) {
        if (sync) {
          callback(resource, error);
        } else {
          dispatch_async(dispatch_get_main_queue(), ^{
            callback(resource, error);
          });
        }
      };
  LocalBundleResult res = [DemoTemplateResourceFetcher readLocalBundleFromResource:request.url];
  if (res.isLocalScheme) {
    completeLocal([[LynxTemplateResource alloc] initWithNSData:res.data], nil);
    return;
  }

  // Handle absolute file paths (e.g. from Sparkling's resolved bundle path)
  if ([request.url hasPrefix:@"/"]) {
    NSData *data = [NSData dataWithContentsOfFile:components.path];
    if (data) {
      completeLocal([[LynxTemplateResource alloc] initWithNSData:data], nil);
    } else {
      NSError *error = [NSError errorWithDomain:@"com.lynx"
                                           code:404
                                       userInfo:@{
                                         NSLocalizedDescriptionKey : [NSString
                                             stringWithFormat:@"File not found: %@", request.url]
                                       }];
      completeLocal(nil, error);
    }
    return;
  }

  NSURL *url = [NSURL URLWithString:request.url];
  NSURLRequest *urlRequest = [NSURLRequest requestWithURL:url
                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                          timeoutInterval:5];

  void (^completeNetwork)(NSData *, NSURLResponse *, NSError *) =
      ^(NSData *data, NSURLResponse *response, NSError *error) {
        if (!error && [response isKindOfClass:[NSHTTPURLResponse class]]) {
          NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
          if (statusCode < 200 || statusCode >= 300) {
            error = [NSError errorWithDomain:NSURLErrorDomain
                                        code:NSURLErrorBadServerResponse
                                    userInfo:@{NSURLErrorKey : url}];
          }
        }
        if (!error && data.length == 0) {
          error = [NSError errorWithDomain:NSURLErrorDomain
                                      code:NSURLErrorZeroByteResource
                                  userInfo:@{NSURLErrorKey : url}];
        }
        callback(error ? nil : [[LynxTemplateResource alloc] initWithNSData:data], error);
      };

  if (sync) {
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block NSData *responseData = nil;
    __block NSError *responseError = nil;
    __block NSURLResponse *urlResponse = nil;
    NSURLSessionDataTask *task = [[NSURLSession sharedSession]
        dataTaskWithRequest:urlRequest
          completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            responseData = data;
            urlResponse = response;
            responseError = error;
            dispatch_semaphore_signal(semaphore);
          }];
    [task resume];
    if (dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC))) {
      [task cancel];
      callback(nil, [NSError errorWithDomain:NSURLErrorDomain
                                        code:NSURLErrorTimedOut
                                    userInfo:@{NSURLErrorKey : url}]);
    } else {
      completeNetwork(responseData, urlResponse, responseError);
    }
    return;
  }

  NSURLSessionDataTask *dataTask = [[NSURLSession sharedSession]
      dataTaskWithRequest:urlRequest
        completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                            NSError *_Nullable error) {
          completeNetwork(data, response, error);
        }];

  [dataTask resume];
}

- (void)fetchSSRData:(LynxResourceRequest *)request
          onComplete:(LynxSSRResourceCompletionBlock)callback {
  NSURL *url = [NSURL URLWithString:request.url];
  NSURLRequest *urlRequest = [NSURLRequest requestWithURL:url
                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                          timeoutInterval:5];

  NSURLSessionDataTask *dataTask = [[NSURLSession sharedSession]
      dataTaskWithRequest:urlRequest
        completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                            NSError *_Nullable error) {
          callback(data, error);
        }];

  [dataTask resume];
}

@end
