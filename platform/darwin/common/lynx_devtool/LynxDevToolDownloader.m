// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDevToolDownloader.h"

@implementation LynxDevToolDownloader

+ (void)download:(NSString*)url withCallback:(downloadCallback)callback {
  NSURL* nsurl = [NSURL URLWithString:url];
  NSURLRequest* request = [NSURLRequest requestWithURL:nsurl
                                           cachePolicy:NSURLRequestReloadIgnoringCacheData
                                       timeoutInterval:2];
  [NSURLConnection
      sendAsynchronousRequest:request
                        queue:[[NSOperationQueue alloc] init]
            completionHandler:^(NSURLResponse* _Nullable response, NSData* _Nullable data,
                                NSError* _Nullable connectionError) {
              if (!connectionError) {
                callback(data, nil);
              } else {
                callback(data, connectionError);
              }
            }];
}

@end
