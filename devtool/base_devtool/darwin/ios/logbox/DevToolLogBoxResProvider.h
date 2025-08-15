// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXRESPROVIDER_H_
#define DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXRESPROVIDER_H_

#import <Foundation/Foundation.h>
#import "DevToolLogBoxHelper.h"

NS_ASSUME_NONNULL_BEGIN

@protocol DevToolLogBoxResProvider

@required
- (NSString*)entryUrlForLogSrc;

- (UIView*)getView;

- (NSDictionary*)logSources;

- (NSString*)logSourceWithFileName:(NSString*)fileName;

@end

NS_ASSUME_NONNULL_END

#endif  // DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXRESPROVIDER_H_
