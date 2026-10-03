// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TemplateProvider.h"

@implementation TemplateProvider

- (nullable NSString*)localBundlePathForURL:(NSString*)url {
  if (![url hasSuffix:@".bundle"] || [url hasPrefix:@"/"]) {
    return nil;
  }

  NSString* relativePath = url;
  while ([relativePath hasPrefix:@"./"]) {
    relativePath = [relativePath substringFromIndex:2];
  }
  if ([relativePath hasPrefix:@"Resource/"]) {
    relativePath = [relativePath substringFromIndex:@"Resource/".length];
  }
  for (NSString* component in relativePath.pathComponents) {
    if ([component isEqualToString:@".."] || [component isEqualToString:@"/"]) {
      return nil;
    }
  }

  NSString* resourceRoot = [[NSBundle mainBundle] pathForResource:@"Resource" ofType:nil];
  if (!resourceRoot) {
    return nil;
  }
  NSString* standardizedRoot = resourceRoot.stringByStandardizingPath;
  NSString* candidate =
      [[standardizedRoot stringByAppendingPathComponent:relativePath] stringByStandardizingPath];
  NSString* rootPrefix = [standardizedRoot stringByAppendingString:@"/"];
  if (![candidate hasPrefix:rootPrefix]) {
    return nil;
  }
  NSFileManager* fileManager = [NSFileManager defaultManager];
  if ([fileManager fileExistsAtPath:candidate]) {
    return candidate;
  }

  // Upstream extension packages commonly navigate to sibling bundles by bare
  // name. Resolve such a name only when exactly one installed extension owns
  // it, avoiding an implicit or order-dependent cross-extension collision.
  if (relativePath.pathComponents.count == 1) {
    NSString* extensionsRoot = [standardizedRoot stringByAppendingPathComponent:@"extensions"];
    NSArray<NSString*>* extensionNames =
        [fileManager contentsOfDirectoryAtPath:extensionsRoot error:nil] ?: @[];
    NSMutableArray<NSString*>* matches = [NSMutableArray new];
    for (NSString* extensionName in extensionNames) {
      NSString* extensionCandidate = [[extensionsRoot stringByAppendingPathComponent:extensionName]
          stringByAppendingPathComponent:relativePath];
      if ([fileManager fileExistsAtPath:extensionCandidate]) {
        [matches addObject:extensionCandidate];
      }
    }
    if (matches.count == 1) {
      return matches.firstObject;
    }
  }
  return nil;
}

- (void)loadTemplateWithUrl:(NSString*)url onComplete:(LynxTemplateLoadBlock)callback {
  // Sparkling uses both bare bundle names and namespaced relative paths.
  // Resolve them inside Resource without allowing a path to escape that root.
  NSString* bundlePath = [self localBundlePathForURL:url];
  if (bundlePath) {
    NSData* data = [NSData dataWithContentsOfFile:bundlePath];
    if (data) {
      dispatch_async(dispatch_get_main_queue(), ^{
        callback(data, nil);
      });
      return;
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
