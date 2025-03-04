// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DemoTemplateResourceFetcher.h"

@implementation DemoTemplateResourceFetcher

// scheme: file://lynx?local://
+ (LocalBundleResult)readLocalBundleFromResource:(NSString *)url {
  LocalBundleResult res = {NO, nil, nil, nil};
  NSURL *source = [NSURL URLWithString:url];
  if ([source.scheme isEqualToString:@"file"]) {
    NSURL *subSourceUrl = [NSURL URLWithString:source.query];
    if ([subSourceUrl.scheme isEqualToString:@"local"]) {
      res.isLocalScheme = YES;
      res.query = [subSourceUrl query];
      NSString *localUrl =
          [NSString stringWithFormat:@"Resource/%@%@", subSourceUrl.host, subSourceUrl.path];
      res.url = [[NSBundle mainBundle] pathForResource:[localUrl stringByDeletingPathExtension]
                                                ofType:@"bundle"];
      if (res.url == nil) {
        NSURL *debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                        withExtension:@"bundle"];
        NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
        localUrl = [NSString stringWithFormat:@"%@%@", subSourceUrl.host, subSourceUrl.path];
        res.url = [bundle pathForResource:[localUrl stringByDeletingPathExtension]
                                   ofType:@"bundle"];
      }
      res.data = [NSData dataWithContentsOfFile:res.url];
    }
  }
  return res;
}

- (void)fetchTemplate:(LynxResourceRequest *)request
           onComplete:(LynxTemplateResourceCompletionBlock)callback {
  LocalBundleResult res = [DemoTemplateResourceFetcher readLocalBundleFromResource:request.url];
  if (res.isLocalScheme) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callback([[LynxTemplateResource alloc] initWithNSData:res.data], nil);
    });
    return;
  }

  NSURL *url = [NSURL URLWithString:request.url];
  NSURLRequest *urlRequest = [NSURLRequest requestWithURL:url
                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                          timeoutInterval:5];

  NSURLSessionDataTask *dataTask = [[NSURLSession sharedSession]
      dataTaskWithRequest:urlRequest
        completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                            NSError *_Nullable error) {
          callback([[LynxTemplateResource alloc] initWithNSData:data], error);
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
