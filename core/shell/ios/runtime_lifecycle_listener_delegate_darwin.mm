// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "lynx/core/shell/ios/runtime_lifecycle_listener_delegate_darwin.h"

#import <Lynx/LynxError.h>
#import <Lynx/LynxRuntimeFullLifecycleListener.h>
#import <Lynx/LynxSubErrorCode.h>
#include "base/include/log/logging.h"

namespace lynx {
namespace shell {
void RuntimeLifecycleListenerDelegateDarwin::OnRuntimeCreate(
    std::shared_ptr<runtime::IVSyncObserver> observer) {
  if (Type() == DelegateType::FULL) {
    @try {
      id<LynxRuntimeFullLifecycleListener> l = id<LynxRuntimeFullLifecycleListener>(_listener);
      [l onRuntimeCreate:&observer];
    } @catch (NSException *exception) {
      OnError(exception);
    }
  }
}

void RuntimeLifecycleListenerDelegateDarwin::OnRuntimeInit(int64_t runtime_id) {
  if (Type() == DelegateType::FULL) {
    @try {
      id<LynxRuntimeFullLifecycleListener> l = id<LynxRuntimeFullLifecycleListener>(_listener);
      [l onRuntimeInit:runtime_id];
    } @catch (NSException *exception) {
      OnError(exception);
    }
  }
}

void RuntimeLifecycleListenerDelegateDarwin::OnAppEnterForeground() {
  if (Type() == DelegateType::FULL) {
    @try {
      id<LynxRuntimeFullLifecycleListener> l = id<LynxRuntimeFullLifecycleListener>(_listener);
      [l onAppEnterForeground];
    } @catch (NSException *exception) {
      OnError(exception);
    }
  }
}
void RuntimeLifecycleListenerDelegateDarwin::OnAppEnterBackground() {
  if (Type() == DelegateType::FULL) {
    @try {
      id<LynxRuntimeFullLifecycleListener> l = id<LynxRuntimeFullLifecycleListener>(_listener);
      [l onAppEnterBackground];
    } @catch (NSException *exception) {
      OnError(exception);
    }
  }
}

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
