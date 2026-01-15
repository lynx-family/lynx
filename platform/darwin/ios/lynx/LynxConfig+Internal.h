// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxConfig.h>
#include "core/runtime/js/bindings/modules/ios/module_factory_darwin.h"

@interface LynxConfig ()
- (std::shared_ptr<lynx::piper::ModuleFactoryDarwin>)moduleFactoryPtr;
- (std::shared_ptr<lynx::piper::ModuleFactoryDarwin>)getSharedModuleFactoryPtr;
- (void)setSharedModuleFactoryPtr:(std::shared_ptr<lynx::piper::ModuleFactoryDarwin>)ptr;
@end
