// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxDevToolPool : NSObject

- (instancetype)initWithURL:(NSString*)url debuggable:(BOOL)debuggable;

- (void)onTemplateBundleCreated:(intptr_t)bundle;

- (void)createDevTool;

- (void)popDevTool;

- (void)onMTSRuntimeCreated;

@end

NS_ASSUME_NONNULL_END
