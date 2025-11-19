// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>
#import <Lynx/LynxLogicExecutor.h>
@class LynxView;
@class LynxTemplateBundle;
@class LynxBackgroundRuntimeOptions;
@class LynxViewGroup;

NS_ASSUME_NONNULL_BEGIN

@interface LynxDefaultLogicExecutor : NSObject <LynxLogicExecutor>
// TODO @dangyingkai delete these two constructor
- (instancetype)initWithRuntimeOptions:(LynxBackgroundRuntimeOptions *)backgroundRuntimeOptions
                         lynxViewGroup:(LynxViewGroup *)lynxViewGroup
                            debuggable:(BOOL)debuggable;

- (instancetype)initWithTemplateBundle:(nullable LynxTemplateBundle *)bundle
              backgroundRuntimeOptions:(LynxBackgroundRuntimeOptions *)backgroundRuntimeOptions
                         lynxViewGroup:(LynxViewGroup *)lynxViewGroup
                            debuggable:(BOOL)debuggable;

- (instancetype)initWithLynxViewGroup:(LynxViewGroup *)lynxViewGroup;
@end

NS_ASSUME_NONNULL_END
