// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOGBOX_LOGBOX_FILE_LOAD_UTILS_H_
#define PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOGBOX_LOGBOX_FILE_LOAD_UTILS_H_

NS_ASSUME_NONNULL_BEGIN

typedef void (^LogBoxFileLoadCallback)(NSString* _Nullable data, NSError* _Nullable error);

@interface LogBoxFileLoadUtils : NSObject
+ (void)loadFileFromLocal:(NSString*)path
                     type:(NSString*)type
               completion:(LogBoxFileLoadCallback)completion;
+ (void)loadFileFromURL:(NSString*)url completion:(LogBoxFileLoadCallback)completion;
@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOGBOX_LOGBOX_FILE_LOAD_UTILS_H_
