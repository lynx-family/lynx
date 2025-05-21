// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "DevToolFileLoadUtils.h"
#import "DevToolDownloader.h"

@implementation DevToolFileLoadUtils

+ (void)loadFileFromBundle:(NSString *)bundleUrl
                  filePath:(NSString *)path
                      type:(NSString *)type
                completion:(DevToolFileLoadCallback)completion {
  if (!path || path.length == 0 || !bundleUrl || bundleUrl.length == 0) {
    completion(
        nil,
        [NSError errorWithDomain:@"com.basedevtool.utils"
                            code:-1
                        userInfo:@{
                          NSLocalizedDescriptionKey : @"Failed to read file from local bundle",
                          NSLocalizedFailureReasonErrorKey : @"path is nil or bundle url is nil"
                        }]);
    return;
  }
  NSURL *debugBundleUrl = [[NSBundle bundleForClass:[self class]] URLForResource:bundleUrl
                                                                   withExtension:@"bundle"];
  if (!debugBundleUrl) {
    completion(nil, [NSError errorWithDomain:@"com.basedevtool.utils"
                                        code:-1
                                    userInfo:@{
                                      NSLocalizedDescriptionKey : @"Failed to read file from local",
                                      NSLocalizedFailureReasonErrorKey : @"bundle not found"
                                    }]);
    return;
  }
  NSError *error;
  NSBundle *debugBundle = [NSBundle bundleWithURL:debugBundleUrl];
  NSString *filePath = [debugBundle pathForResource:path ofType:type];
  NSString *content = [NSString stringWithContentsOfFile:filePath
                                                encoding:NSUTF8StringEncoding
                                                   error:&error];
  if (!error) {
    completion(content, nil);
  } else {
    completion(nil, error);
  }
}

+ (void)loadFileFromURL:(NSString *)url completion:(DevToolFileLoadCallback)completion {
  if (!url || url.length == 0) {
    completion(nil, [NSError errorWithDomain:@"com.lynxdevtool.logbox"
                                        code:-1
                                    userInfo:@{
                                      NSLocalizedDescriptionKey : @"Failed to read file from url",
                                      NSLocalizedFailureReasonErrorKey : @"url is nil"
                                    }]);
    return;
  }
  [DevToolDownloader download:url
                 withCallback:^(NSData *_Nullable data, NSError *_Nullable error) {
                   if (!error) {
                     NSString *content = [[NSString alloc] initWithData:data
                                                               encoding:NSUTF8StringEncoding];
                     completion(content, nil);
                   } else {
                     completion(nil, error);
                   }
                 }];
}

@end
