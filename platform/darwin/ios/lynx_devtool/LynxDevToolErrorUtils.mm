// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxDevToolErrorUtils.h>

#include "core/runtime/common/lynx_console_helper.h"

@implementation LynxDevToolErrorUtils

+ (NSString *)getKeyMessage:(LynxError *)error {
  if (!error || ![error isValid]) {
    return @"";
  }
  NSMutableString *msg = [[NSMutableString alloc] init];
  [msg appendFormat:@"code: %ld\n", [error getSubCode]];
  [msg appendFormat:@"message: %@\n", error.summaryMessage];
  if (error.rootCause) {
    [msg appendFormat:@"root_cause: %@\n", error.rootCause];
  }
  NSDictionary *contextInfo = [error getContextInfo];
  for (NSString *info in contextInfo) {
    [msg appendFormat:@"%@: %@\n", info, [contextInfo objectForKey:info]];
  }
  if (error.fixSuggestion && [error.fixSuggestion length] > 0) {
    [msg appendFormat:@"fix_suggestion: %@", error.fixSuggestion];
  }
  return msg;
}

+ (NSInteger)intValueFromErrorLevelString:(NSString *)levelStr {
  int level = lynx::piper::CONSOLE_LOG_ERROR;
  if (!levelStr) {
    return level;
  }
  if ([levelStr isEqualToString:LynxErrorLevelWarn]) {
    level = lynx::piper::CONSOLE_LOG_WARNING;
  }
  return level;
}

@end
