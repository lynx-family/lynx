// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxConfig.h"
#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"

@interface LynxConfig ()
- (std::shared_ptr<lynx::piper::ModuleFactoryDarwin>)moduleFactoryPtr;
@end
