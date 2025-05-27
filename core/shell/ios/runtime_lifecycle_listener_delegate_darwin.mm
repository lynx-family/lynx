// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/ios/runtime_lifecycle_listener_delegate_darwin.h"

#import <Lynx/LynxError.h>
#import <Lynx/LynxRuntimeLifecycleListener.h>
#import <Lynx/LynxSubErrorCode.h>
#include "base/include/log/logging.h"

namespace lynx {
namespace shell {

void RuntimeLifecycleListenerDelegateDarwin::OnRuntimeAttach(Napi::Env current_napi_env) {
  @try {
    [_listener onRuntimeAttach:current_napi_env];
  } @catch (NSException *exception) {
    OnError(exception);
  }
}

void RuntimeLifecycleListenerDelegateDarwin::OnRuntimeDetach() {
  @try {
    [_listener onRuntimeDetach];
  } @catch (NSException *exception) {
    OnError(exception);
  }
}

void RuntimeLifecycleListenerDelegateDarwin::OnError(NSException *e) {
  LOGE("Exception happened in RuntimeLifecycle exec! Error message: " << [e name] << " reason:" <<
       [e reason] << " userInfo:" << [e userInfo]);
  if (_error_handler) {
    [_error_handler
        onErrorOccurred:
            [LynxError lynxErrorWithCode:ECLynxBTSLifecycleListenerErrorException
                                 message:[e reason]
                           fixSuggestion:@"check your client RuntimeLifecycleListener implement."
                                   level:LynxErrorLevelError]];
  }
}

}  // namespace shell
}  // namespace lynx
