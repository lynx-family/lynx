// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/module_delegate_impl.h"

#include <utility>

namespace lynx {
namespace shell {
int64_t ModuleDelegateImpl::RegisterJSCallbackFunction(piper::Function func) {
  // TODO(heshan):now not support copyable lambda for std::function, cannot use
  // ActSync, tricky... can ensure call on js thread
  auto* runtime = runtime_actor_->Impl();
  if (runtime == nullptr) {
    return piper::ModuleCallback::kInvalidCallbackId;
  }
  return runtime->RegisterJSCallbackFunction(std::move(func));
}

void ModuleDelegateImpl::CallJSCallback(
    const std::shared_ptr<piper::ModuleCallback>& callback,
    int64_t id_to_delete) {
  runtime_actor_->Act([callback, id_to_delete](auto& runtime) {
    runtime->CallJSCallback(callback, id_to_delete);
  });
}

void ModuleDelegateImpl::OnErrorOccurred(base::LynxError error) {
  runtime_actor_->Act([error = std::move(error)](auto& runtime) mutable {
    runtime->OnErrorOccurred(std::move(error));
  });
}

void ModuleDelegateImpl::OnMethodInvoked(const std::string& module_name,
                                         const std::string& method_name,
                                         int32_t code) {
  runtime_actor_->Act([module_name, method_name, code](auto& runtime) {
    runtime->OnModuleMethodInvoked(module_name, method_name, code);
  });
}

void ModuleDelegateImpl::FlushJSBTiming(piper::NativeModuleInfo timing) {
  runtime_actor_->Act([timing = std::move(timing)](auto& runtime) mutable {
    if (tasm::LynxEnv::GetInstance().EnableAsyncJSBTiming()) {
      tasm::report::EventTracker::OnEvent(
          [timing = std::move(timing)](tasm::report::MoveOnlyEvent& event) {
            event.SetName("lynxsdk_jsb_timing");
            event.SetProps("jsb_module_name", timing.module_name_);
            event.SetProps("jsb_method_name", timing.method_name_);
            event.SetProps("jsb_name", timing.method_first_arg_name_);
            event.SetProps("jsb_protocol_version", 0);
            event.SetProps("jsb_bridgesdk", "lynx");
            event.SetProps("jsb_status_code",
                           static_cast<int32_t>(timing.status_code_));
            event.SetProps("jsb_call", timing.jsb_call_);
            event.SetProps("jsb_func_call", timing.jsb_func_call_);
            event.SetProps("jsb_func_convert_params",
                           timing.jsb_func_convert_params_);
            event.SetProps("jsb_func_platform_method",
                           timing.jsb_func_platform_method_);
            event.SetProps("jsb_callback_thread_switch",
                           timing.jsb_callback_thread_switch_);
            event.SetProps("jsb_callback_thread_switch_waiting",
                           timing.jsb_callback_thread_switch_waiting_);
            event.SetProps("jsb_callback_call", timing.jsb_callback_call_);
            event.SetProps("jsb_callback_convert_params",
                           timing.jsb_callback_convert_params_);
            event.SetProps("jsb_callback_invoke", timing.jsb_callback_invoke_);
            event.SetProps("jsb_func_call_start", timing.jsb_func_call_start_);
            event.SetProps("jsb_func_call_end", timing.jsb_func_call_end_);
            event.SetProps("jsb_callback_thread_switch_start",
                           timing.jsb_callback_thread_switch_start_);
            event.SetProps("jsb_callback_thread_switch_end",
                           timing.jsb_callback_thread_switch_end_);
            event.SetProps("jsb_callback_call_start",
                           timing.jsb_callback_call_start_);
            event.SetProps("jsb_callback_call_end",
                           timing.jsb_callback_call_end_);
          });
    } else {
      runtime->FlushJSBTiming(std::move(timing));
    }
  });
}

void ModuleDelegateImpl::RunOnJSThread(base::closure func) {
  runtime_actor_->Act([func = std::move(func)](auto& runtime) { func(); });
}

void ModuleDelegateImpl::RunOnPlatformThread(base::closure func) {
  if (facade_actor_) {
    facade_actor_->Act([func = std::move(func)](auto& facade) { func(); });
  }
}

}  // namespace shell
}  // namespace lynx
