// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/base/darwin/lynx_env_darwin.h"

#include "core/base/threading/task_runner_manufactor.h"
// TODO(zhengsenyao): move lynx_env_darwin.mm out of //lynx/core/base/
#import "LynxEnv.h"  // nogncheck

namespace lynx {
namespace tasm {

void LynxEnvDarwin::onPiperInvoked(const std::string& module_name, const std::string& method_name,
                                   const std::string& param_str, const std::string& url,
                                   const std::string& invoke_session) {
  [[LynxEnv sharedInstance] onPiperInvoked:[NSString stringWithUTF8String:module_name.c_str()]
                                    method:[NSString stringWithUTF8String:method_name.c_str()]
                                  paramStr:[NSString stringWithUTF8String:param_str.c_str()]
                                       url:[NSString stringWithUTF8String:url.c_str()]
                                 sessionID:[NSString stringWithUTF8String:invoke_session.c_str()]];
}

void LynxEnvDarwin::onPiperResponsed(const std::string& module_name, const std::string& method_name,
                                     const std::string& url, NSDictionary* response,
                                     const std::string& invoke_session) {
  [[LynxEnv sharedInstance]
      onPiperResponsed:module_name.empty() ? @""
                                           : [NSString stringWithUTF8String:module_name.c_str()]
                method:method_name.empty() ? @""
                                           : [NSString stringWithUTF8String:method_name.c_str()]
                   url:url.empty() ? @"" : [NSString stringWithUTF8String:url.c_str()]
              response:response ?: @{}
             sessionID:invoke_session.empty()
                           ? @""
                           : [NSString stringWithUTF8String:invoke_session.c_str()]];
}

void LynxEnvDarwin::initNativeUIThread() {
  // init ui thread native loop
  if (NSThread.isMainThread) {
    base::UIThread::Init();
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      base::UIThread::Init();
    });
  }
}

}  // namespace tasm
}  // namespace lynx
