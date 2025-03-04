// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxViewBuilder.h"

#import "LynxBackgroundRuntime+Internal.h"
#import "LynxUIRendererProtocol.h"

@interface LynxViewBuilder ()

@property(nonatomic, nonnull) LynxBackgroundRuntimeOptions* lynxBackgroundRuntimeOptions;

@property(nonatomic, nonnull) id<LynxUIRendererProtocol> lynxUIRenderer;

- (NSDictionary* _Nonnull)getLynxResourceProviders;

- (NSDictionary* _Nonnull)getBuilderRegisteredAliasFontMap;

@end
