// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxNativeLayoutNode;
@protocol LynxMarkdownResourceLoaderHost;

@interface LynxMarkdownInlineViewHandle : NSObject <IMarkdownPlatformViewHandle>

- (instancetype)initWithLayoutNode:(LynxNativeLayoutNode *)layoutNode
                              host:(id<LynxMarkdownResourceLoaderHost>)host;

@end

NS_ASSUME_NONNULL_END
