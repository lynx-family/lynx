// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class LynxDevtool;

/**
 * base class for LynxTemplateRender and LynxFrameRender, provide common methods to build native
 */
@interface LynxBaseTemplateRender : NSObject

@property(nonatomic, strong, nonnull) LynxDevtool* devTool;
@property(nonatomic, assign) BOOL enableJSRuntime;
@property(nonatomic, strong, nullable) NSMutableDictionary<NSString*, id>* lepusModulesClasses;

- (void)setNeedPendingUIOperation:(BOOL)needPendingUIOperation;

@end
