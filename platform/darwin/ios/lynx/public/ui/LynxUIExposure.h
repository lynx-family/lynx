// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRootUI.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIExposure : NSObject

- (void)setRootUI:(LynxRootUI *)rootUI;
- (BOOL)addLynxUI:(LynxUI *)ui
    withUniqueIdentifier:(NSString *_Nullable)uniqueID
               extraData:(NSDictionary *_Nullable)data
              useOptions:(NSDictionary *_Nullable)options;
- (void)removeLynxUI:(LynxUI *)ui withUniqueIdentifier:(NSString *_Nullable)uniqueID;
- (void)willMoveToWindow:(BOOL)windowIsNil;
- (void)didMoveToWindow:(BOOL)windowIsNil;
- (void)destroyExposure;
- (void)addExposureToRunLoop;
- (void)stopExposure:(NSDictionary *)options;
- (void)resumeExposure;
- (void)setObserverFrameRateDynamic:(NSDictionary *)options;

@end

NS_ASSUME_NONNULL_END
