// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/bindings/modules/lepus_module_callback.h"

#include "base/include/lynx_actor.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace lepus {

lepus::Value LepusModuleCallback::GetArgs() {
  if (args_) {
    return pub::ValueUtils::ConvertValueToLepusValue(*args_);
  }
  return lepus::Value();
}

LepusModuleDelegate::LepusModuleDelegate() {
  value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
}

void LepusModuleDelegate::InvokeCallback(
    const std::shared_ptr<piper::LynxModuleCallback>& callback,
    base::MoveOnlyClosure<bool> invoke_pre_func) {
  if (engine_actor_ != nullptr) {
    engine_actor_->Act([callback](auto& engine) -> void {
      if (callback->GetType() == piper::LynxModuleCallback::Type::LEPUS) {
        // safe cast to lepus callback
        std::shared_ptr<LepusModuleCallback> lepus_callback =
            std::static_pointer_cast<LepusModuleCallback>(callback);
        engine->InvokeLepusCallback(
            static_cast<int32_t>(lepus_callback->CallbackId()), "",
            lepus_callback->GetArgs(), true);
      } else {
        LOGE("NativeModules"
             << "module callback type error , expected type lepus");
      }
    });
  } else {
    LOGE("NativeModules"
         << "Engine is destroyed, skip invoke lepus callback");
  }
}

void LepusModuleDelegate::SetEngineActor(
    const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& engine_actor) {
  if (engine_actor != nullptr) {
    engine_actor_ = engine_actor;
  }
}

void LepusModuleDelegate::SetFacadeActor(
    const std::shared_ptr<shell::LynxActor<shell::NativeFacade>>&
        facade_actor) {
  if (facade_actor != nullptr) {
    facade_actor_ = facade_actor;
  }
}

void LepusModuleDelegate::OnErrorOccurred(const std::string& module_name,
                                          const std::string& method_name,
                                          base::LynxError error) {
  LOGE("NativeModules"
       << "ErrorOccur, module_name: " << module_name << ", method_name: "
       << method_name << ", error: " << error.error_message_);
  if (facade_actor_ != nullptr) {
    facade_actor_->Act([error = std::move(error)](auto& facade) {
      facade->ReportError(std::move(error));
    });
  }
}
}  // namespace lepus
}  // namespace lynx
