// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <ServalMarkdown/IMarkdownEventDelegate.h>
#import <ServalMarkdown/IMarkdownExposureDelegate.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxMarkdownEventDispatcherHost <NSObject>

- (void)dispatchMarkdownEvent:(NSString *)name detail:(nullable NSDictionary *)detail;
- (NSString *)markdownParseEndContentID;
- (NSInteger)markdownContentLength;

@end

@interface LynxMarkdownEventDispatcher
    : NSObject <IMarkdownEventDelegate, IMarkdownExposureDelegate>

- (instancetype)initWithHost:(id<LynxMarkdownEventDispatcherHost>)host;

@end

NS_ASSUME_NONNULL_END
