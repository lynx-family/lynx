// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/bindings/modules/lynx_lepus_module.h"

#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/lepus/bindings/modules/lepus_module_callback.h"
#include "core/runtime/lepus/bindings/modules/lynx_lepus_module_manager.h"
#include "core/runtime/lepus/tasks/lepus_callback_manager.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"

namespace lynx {
namespace lepus {

Value LynxLepusModule::InvokeMethod(Context* context,
                                    const std::string& method_name,
                                    const lepus::Value* args, size_t count) {
  LOGI("NativeModules"
       << "LynxLepusModule::InvokeMethod: method_name: " << method_name.c_str()
       << " count: " << count);
  if (native_module_ == nullptr) {
    constexpr const static char* error_msg =
        "NativeModules: LynxLepusModule::InvokeMethod: NativeModule is null";
    execute_delegate_->OnErrorOccurred(
        name_, method_name,
        base::LynxError(error::E_NATIVE_MODULES_COMMON_MODULE_NOT_FOUND,
                        error_msg));
    return Value();
  }

  auto args_array = value_factory_->CreateArray();
  piper::CallbackMap callback_map;
  for (size_t i = 0; i < count; i++) {
    if (args[i].IsJSFunction() || args[i].IsClosure()) {
      std::unique_ptr<lepus::Value> callback_closure =
          std::make_unique<lepus::Value>(args[i]);
      auto callback_id = context->GetCallbackManager()->CacheTask(
          context, std::move(callback_closure));
      auto callback = std::make_shared<LepusModuleCallback>(callback_id);
      // callback->SetModuleName(name_);
      // callback->SetMethodName(method_name);
      // callback->SetCallbackFlowId(callback_flow_id);
      // callback->SetFirstArg(first_arg_str);
      callback_map.emplace(static_cast<int>(i), std::move(callback));
      args_array->PushInt64ToArray(callback_id);
    } else {
      args_array->PushValueToArray(pub::ValueImplLepus(args[i]));
    }
  }
  // really invoke method
  auto ret = native_module_->InvokeMethod(method_name, std::move(args_array),
                                          count, callback_map);
  if (ret.has_value()) {
    auto result =
        pub::ValueUtils::ConvertValueToLepusValue(*(ret.value().get()));
    return result;
  } else {
    std::string error =
        "LynxLepusModule::InvokeMethod: InvokeMethod failed , module_name: " +
        name_ + " method_name: " + method_name + "error: " + ret.error();
    LOGE("NativeModules" << error);
    return Value();
  }
  return Value();
}

Value LynxLepusModule::ToLepusValue(Context* context) {
  return tasm::Utils::CreateLepusModule(context, lepus::Value(fml::Ref(this)));
}

LynxLepusModule* LynxLepusModule::ToRuntimeValue(const Value& lepus_value) {
  if (!lepus_value.IsObject()) {
    return nullptr;
  }

  auto runtime_property =
      lepus_value.GetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy));
  if (!runtime_property.IsRefCounted()) {
    return nullptr;
  }
  // static cast
  LynxLepusModule* runtime_module =
      fml::static_ref_ptr_cast<LynxLepusModule>(runtime_property.RefCounted())
          .get();
  return runtime_module;
}

}  // namespace lepus
}  // namespace lynx
