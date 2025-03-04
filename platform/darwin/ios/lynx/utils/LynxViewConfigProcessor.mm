// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxViewConfigProcessor.h"
#import "LynxLog.h"

@implementation LynxViewConfigProcessor

+ (void)processorMap:(NSMutableDictionary *)dictionary
     lynxViewBuilder:(LynxViewBuilder *)lynxViewBuilder {
  if (dictionary == nil || [dictionary count] == 0) {
    return;
  }
  id multiAsyncThread = [dictionary objectForKey:KEY_LYNX_ENABLE_MULTI_ASYNC_THREAD];
  if ([multiAsyncThread isKindOfClass:NSNumber.class]) {
    lynxViewBuilder.enableMultiAsyncThread = [(NSNumber *)multiAsyncThread intValue] == 1;
  }

  // parse runtime type
  id runtimeType = [dictionary objectForKey:KEY_LYNX_RUNTIME_TYPE];
  if ([runtimeType isKindOfClass:NSString.class]) {
    NSString *runtimeTypeStr = (NSString *)runtimeType;
    if ([@"jsc" caseInsensitiveCompare:runtimeTypeStr] == NSOrderedSame) {
      lynxViewBuilder.backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeJSC;
    } else if ([@"quickjs" caseInsensitiveCompare:runtimeTypeStr] == NSOrderedSame) {
      lynxViewBuilder.backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeQuickjs;
    } else if ([@"v8" caseInsensitiveCompare:runtimeTypeStr] == NSOrderedSame) {
      lynxViewBuilder.backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeV8;
    }
  }
  // parse if enable bytecode
  id enableBytecode = [dictionary objectForKey:KEY_LYNX_ENABLE_BYTECODE];
  if ([enableBytecode respondsToSelector:@selector(boolValue)]) {
    lynxViewBuilder.enableBytecode = [enableBytecode boolValue];
  }
  // parse if has bytecode url
  id bytecodeUrl = [dictionary objectForKey:KEY_LYNX_BYTECODE_URL];
  if ([bytecodeUrl isKindOfClass:NSString.class]) {
    lynxViewBuilder.bytecodeUrl = (NSString *)bytecodeUrl;
  }
}

@end
