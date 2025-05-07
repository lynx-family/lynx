// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "DevToolLogBoxEnv.h"

@implementation DevToolLogBoxEnv {
  NSMutableDictionary<NSString *, LoadErrorParserBlock> *_errorParserMap;
}

+ (DevToolLogBoxEnv *)sharedInstance {
  static DevToolLogBoxEnv *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[DevToolLogBoxEnv alloc] init];
  });
  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _errorParserMap = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)registerErrorParserLoader:(LoadErrorParserBlock)loadBlock
                    withNamespace:(NSString *)errNamespace {
  if (errNamespace == nil || errNamespace.length == 0 ||
      [_errorParserMap objectForKey:errNamespace] || loadBlock == nil) {
    return;
  }
  @synchronized(_errorParserMap) {
    if (![_errorParserMap objectForKey:errNamespace]) {
      [_errorParserMap setObject:loadBlock forKey:errNamespace];
    }
  }
}

- (void)loadErrorParser:(NSString *)errNamespace completion:(LogBoxFileLoadCallback)completion {
  LoadErrorParserBlock loadBlock = [_errorParserMap objectForKey:errNamespace];
  if (loadBlock == nil) {
    completion(nil, [NSError errorWithDomain:@"com.lynxdevtool.logbox"
                                        code:-1
                                    userInfo:@{
                                      NSLocalizedDescriptionKey : @"Failed to load error parser",
                                      NSLocalizedFailureReasonErrorKey :
                                          @"Cannot find loader for namespace"
                                    }]);
    return;
  }
  loadBlock(completion);
}

@end
