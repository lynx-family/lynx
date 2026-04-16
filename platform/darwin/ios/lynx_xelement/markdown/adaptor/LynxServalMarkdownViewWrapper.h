// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <ServalMarkdown/ServalMarkdownView.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxMarkdownShadowNode;

@interface LynxServalMarkdownViewWrapper : ServalMarkdownView

@property(nonatomic, weak, readonly, nullable) LynxMarkdownShadowNode *shadowNode;

- (instancetype)initWithShadowNode:(nullable LynxMarkdownShadowNode *)shadowNode;

@end

NS_ASSUME_NONNULL_END
