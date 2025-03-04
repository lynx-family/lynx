// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_LYNX_MODULE_INTERCEPTOR_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_LYNX_MODULE_INTERCEPTOR_H_

#import <Foundation/Foundation.h>
#include "core/public/pub_value.h"
#include "core/runtime/jsi/jsi.h"

#include <string>

namespace lynx {
namespace piper {

class LynxModuleInterceptor {
 public:
  static std::string GetJSBFuncName(id instance, const pub::Value *first_arg);

  static void CheckModuleIfNeed(id module, NSMutableDictionary *temp, NSDictionary *extra);
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_LYNX_MODULE_INTERCEPTOR_H_
