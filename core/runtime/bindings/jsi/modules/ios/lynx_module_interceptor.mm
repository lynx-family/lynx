// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/runtime/bindings/jsi/modules/ios/lynx_module_interceptor.h"
#import <Foundation/Foundation.h>

namespace lynx {
namespace piper {

std::string LynxModuleInterceptor::GetJSBFuncName(id instance, const pub::Value *first_arg) {
  // Used to get the first argument as the method name if need.
  return {};
}

void LynxModuleInterceptor::CheckModuleIfNeed(id module, NSMutableDictionary *temp,
                                              NSDictionary *extra) {
  // check module
}

}  // namespace piper
}  // namespace lynx
