// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/shared_data/lynx_white_board.h"

@interface LynxGroup ()

@property(nonatomic, readonly) std::shared_ptr<lynx::tasm::WhiteBoard> whiteBoard;

// Bunch of LynxView have the same group name wil share one JS Context
+ (NSString*)groupNameForLynxGroupOrDefault:(LynxGroup*)group;

// Bunch of LynxView have the same JS thread name wil run on same JS Thread
+ (NSString*)jsThreadNameForLynxGroupOrDefault:(LynxGroup*)group;

@end
