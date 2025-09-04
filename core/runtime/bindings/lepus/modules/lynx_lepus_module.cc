// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/modules/lynx_lepus_module.h"

#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/lepus/modules/lynx_lepus_module_manager.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

Value LynxLepusModule::InvokeMethod(const std::string& method_name,
                                    const lepus::Value* args, size_t count) {
  LOGI("NativeModules"
       << "LynxLepusModule::InvokeMethod: method_name: " << method_name.c_str()
       << " count: " << count);
  if (native_module_ == nullptr) {
    LOGE("NativeModules"
         << "LynxLepusModule::InvokeMethod: NativeModule is null");
    return Value();
  }

  auto args_array = value_factory_->CreateArray();
  piper::CallbackMap callback_map;
  for (size_t i = 0; i < count; i++) {
    if (args->IsClosure()) {
      // TODO(zhangqun.29) support closure data type & delete this
      args_array->PushValueToArray(pub::ValueImplLepus(lepus::Value()));
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
    LOGE("NativeModules"
         << "LynxLepusModule::InvokeMethod: InvokeMethod failed , module_name: "
         << name_ << " method_name: " << method_name
         << "error: " << ret.error());
    // TODO: Support OnErrorOccurred
    return Value();
  }
  return Value();
}

Value LynxLepusModule::ToLepusValue(Context* context) {
  lepus::Value lepus_value = lepus::LEPUSValueHelper::CreateObject(context);
  lepus_value.SetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy),
                          lepus::Value(fml::Ref(this)));
  if (context->IsVMContext()) {
#if !ENABLE_JUST_LEPUSNG
    tasm::Utils::RegisterMethodToLepusModule(context, lepus_value);
#endif
  } else {
    tasm::Utils::RegisterNGMethodToLepusModule(context, lepus_value);
  }
  return lepus_value;
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
