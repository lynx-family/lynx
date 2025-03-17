// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>

NS_ASSUME_NONNULL_BEGIN

@interface LepusDebugInfoHelper : NSObject

@property(readwrite) NSString* debugInfoUrl;

- (instancetype)init;

- (std::string)getDebugInfo:(const std::string&)url;

@end

NS_ASSUME_NONNULL_END
