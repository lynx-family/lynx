// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TemplateProvider.h"

@implementation TemplateProvider

- (void)loadTemplateWithUrl:(NSString*)url onComplete:(LynxTemplateLoadBlock)callback {
  // Try loading as a local bundle from the Resource directory first.
  // Sparkling passes bare bundle names like "homepage.lynx.bundle".
  if ([url hasSuffix:@".bundle"]) {
    NSString* bundleName = [url stringByDeletingPathExtension];
    NSString* bundlePath = [[NSBundle mainBundle] pathForResource:bundleName
                                                           ofType:@"bundle"
                                                      inDirectory:@"Resource"];
    if (bundlePath) {
      NSData* data = [NSData dataWithContentsOfFile:bundlePath];
      if (data) {
        dispatch_async(dispatch_get_main_queue(), ^{
          callback(data, nil);
        });
        return;
      }
    }
  }

  // Fallback: load from network URL
  NSString* encodeUrl =
      [url stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet
                                                                  URLFragmentAllowedCharacterSet]];
  NSURL* nsUrl = [NSURL URLWithString:encodeUrl];
  NSURLSessionDataTask* task = [[NSURLSession sharedSession]
        dataTaskWithURL:nsUrl
      completionHandler:^(NSData* _Nullable data, NSURLResponse* _Nullable response,
                          NSError* _Nullable error) {
        dispatch_async(dispatch_get_main_queue(), ^{
          if (error) {
            callback(data, error);
          } else if (!data) {
            NSMutableDictionary* details = [NSMutableDictionary new];
            NSString* errorMsg = [NSString stringWithFormat:@"data from %@ is nil!", url];
            [details setObject:errorMsg forKey:NSLocalizedDescriptionKey];
            NSError* data_error = [NSError errorWithDomain:@"com.lynx" code:200 userInfo:details];
            callback(nil, data_error);
          } else {
            callback(data, nil);
          }
        });
      }];
  [task resume];
}

@end
