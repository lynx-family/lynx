// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// GeneralInfo props name:
// The last loaded URL in this lynxView, will be updated when lynxView render new template.
FOUNDATION_EXTERN NSString *const kPropURL;
// the relative_path would be equivalent to the url to remove applicationExternalCacheDir,
// applicationFilesDir and LocalDir.
// It can be more accurate to filter info by relative_path than by url on the tea platform.
FOUNDATION_EXTERN NSString *const kPropRelativePath;
// The last thread strategy this lynxView is using, will be updated when the lynxView is init.
FOUNDATION_EXTERN NSString *const kPropThreadMode;
// Enable SSR.
FOUNDATION_EXPORT NSString *const kPropEnableSSR;

/**
 * Class hold some info like templateURL, thread strategy, pageConfig and etc.
 * It's used to report some common useful parameter when report event.
 * Mainly converted to JSONObject by toJSONObject() method,
 * and used as the third argument in API below:
 */
@interface LynxEventReporterUtils : NSObject

/// Get relative path of template url.
+ (NSString *)relativePathForURL:(NSString *)urlStr;

@end

NS_ASSUME_NONNULL_END
