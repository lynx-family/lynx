// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxConvertUtils.h"

@implementation LynxConvertUtils
+ (NSString *)convertToJsonData:(NSDictionary *)dict {
  NSError *error;
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:dict options:kNilOptions error:&error];
  NSString *jsonString;
  if (!jsonData) {
    NSLog(@"%@", error);
  } else {
    jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  }
  return jsonString;
}

@end
