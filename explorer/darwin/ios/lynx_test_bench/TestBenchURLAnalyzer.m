// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchURLAnalyzer.h"

@implementation TestBenchURLAnalyzer

+ (NSString*)getQueryStringParameter:(NSURL*)url forKey:(NSString*)key {
  NSURLComponents* urlComponents = [NSURLComponents componentsWithURL:url
                                              resolvingAgainstBaseURL:NO];
  NSArray* queryItems = urlComponents.queryItems;

  if ([queryItems count] == 0) {
    return nil;
  }

  NSPredicate* predicate = [NSPredicate predicateWithFormat:@"name=%@", key];
  NSURLQueryItem* queryItem = [[queryItems filteredArrayUsingPredicate:predicate] firstObject];

  return queryItem.value;
}

+ (BOOL)getQueryBooleanParameter:(NSURL*)url forKey:(NSString*)key defaultValue:(BOOL)defaultValue {
  NSString* val = [self getQueryStringParameter:url forKey:key];
  return (val != nil && val.length > 0) ? [val boolValue] : defaultValue;
}

@end
