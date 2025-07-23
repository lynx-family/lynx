// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_RESOURCE_RESPONSE_HANDLER_IN_LEPUS_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_RESOURCE_RESPONSE_HANDLER_IN_LEPUS_H_

#include <future>
#include <memory>
#include <optional>
#include <string>

#include "base/include/value/base_value.h"
#include "core/runtime/bindings/common/resource/response_handler_proxy.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {

class ResponseHandlerInLepus : public runtime::ResponseHandlerProxy,
                               public lepus::RefCounted {
 public:
  ResponseHandlerInLepus(
      runtime::ResponseHandlerProxy::Delegate& delegate, const std::string& url,
      const std::shared_ptr<
          runtime::ResponsePromise<tasm::BundleResourceInfo>>&);

  virtual ~ResponseHandlerInLepus() override = default;

  static lepus::Value GetBindingObject(
      lepus::Context* context,
      fml::RefPtr<tasm::ResponseHandlerInLepus>& handler);

  static ResponseHandlerInLepus* GetResponseHandlerFromLepusValue(
      const lepus::Value& binding_object);

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kOtherType;
  };
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_RESOURCE_RESPONSE_HANDLER_IN_LEPUS_H_
