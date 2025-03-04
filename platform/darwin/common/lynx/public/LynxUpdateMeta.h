// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXUPDATEMETA_H_
#define DARWIN_COMMON_LYNX_LYNXUPDATEMETA_H_

#import "LynxTemplateData.h"

@interface LynxUpdateMeta : NSObject
@property(nonatomic, strong, nullable) LynxTemplateData* data;
@property(nonatomic, strong, nullable) LynxTemplateData* globalProps;
@end

#endif  // DARWIN_COMMON_LYNX_LYNXUPDATEMETA_H_
