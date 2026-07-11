// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxSettingPlatformDarwinCallback)(NSString* resultJson,
                                                  NSString* _Nullable errorMessage);

@interface LynxSettingPlatformDarwin : NSObject

+ (void)handleMethod:(NSString*)method
                 key:(NSString*)key
               value:(NSString*)value
            callback:(LynxSettingPlatformDarwinCallback)callback;

@end

NS_ASSUME_NONNULL_END
