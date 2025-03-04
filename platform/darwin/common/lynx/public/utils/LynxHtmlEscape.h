// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_UTILS_LYNXHTMLESCAPE_H_
#define DARWIN_COMMON_LYNX_UTILS_LYNXHTMLESCAPE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSString (LynxHtmlEscape)

- (NSString *)stringByUnescapingFromHtml;

@end
NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_UTILS_LYNXHTMLESCAPE_H_
