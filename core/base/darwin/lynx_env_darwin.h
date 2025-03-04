// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DARWIN_LYNX_ENV_DARWIN_H_
#define CORE_BASE_DARWIN_LYNX_ENV_DARWIN_H_

#import <Foundation/Foundation.h>
#include <string>

namespace lynx {
namespace tasm {

class LynxEnvDarwin {
 public:
  LynxEnvDarwin() = delete;
  ~LynxEnvDarwin() = delete;

  static void onPiperInvoked(const std::string& module_name, const std::string& method_name,
                             const std::string& param_str, const std::string& url,
                             const std::string& invoke_session);
  static void onPiperResponsed(const std::string& module_name, const std::string& method_name,
                               const std::string& url, NSDictionary* response,
                               const std::string& invoke_session);

  static void initNativeUIThread();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_BASE_DARWIN_LYNX_ENV_DARWIN_H_
