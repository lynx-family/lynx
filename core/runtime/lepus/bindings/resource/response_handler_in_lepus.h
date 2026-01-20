// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BINDINGS_RESOURCE_RESPONSE_HANDLER_IN_LEPUS_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_RESOURCE_RESPONSE_HANDLER_IN_LEPUS_H_

#include <future>
#include <memory>
#include <optional>
#include <string>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/value/base_value.h"
#include "core/runtime/common/bindings/resource/response_handler_proxy.h"
#include "core/runtime/lepus/context.h"

namespace lynx {
namespace tasm {

class ResponseHandlerInLepus
    : public runtime::ResponseHandlerProxy,
      public lepus::RefCounted,
      public fml::EnableWeakFromThis<ResponseHandlerInLepus> {
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

  void AddResourceListener(
      base::MoveOnlyClosure<void, tasm::BundleResourceInfo> closure) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_RESOURCE_RESPONSE_HANDLER_IN_LEPUS_H_
