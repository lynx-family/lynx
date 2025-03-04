// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxContextModule.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxSetModule : NSObject <LynxContextModule>

- (instancetype)initWithLynxContext:(LynxContext *)context;

- (void)switchKeyBoardDetect:(BOOL)arg;

- (BOOL)getEnableLayoutOnly;

- (void)switchEnableLayoutOnly:(BOOL)arg;

- (BOOL)getAutoResumeAnimation;

- (void)setAutoResumeAnimation:(BOOL)arg;

- (BOOL)getEnableNewTransformOrigin;

- (void)setEnableNewTransformOrigin:(BOOL)arg;

- (NSMutableDictionary *)setUpSettingsDict;

@end

NS_ASSUME_NONNULL_END
