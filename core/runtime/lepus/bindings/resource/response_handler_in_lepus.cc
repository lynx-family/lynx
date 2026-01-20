// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/bindings/resource/response_handler_in_lepus.h"

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/common/bindings/resource/response_promise.h"
#include "core/runtime/js/bindings/js_app.h"
#include "core/runtime/lepusng/jsvalue_helper.h"

namespace lynx {
namespace tasm {

ResponseHandlerInLepus::ResponseHandlerInLepus(
    runtime::ResponseHandlerProxy::Delegate& delegate, const std::string& url,
    const std::shared_ptr<runtime::ResponsePromise<tasm::BundleResourceInfo>>&
        promise)
    : runtime::ResponseHandlerProxy(delegate, url, promise) {}

lepus::Value ResponseHandlerInLepus::GetBindingObject(
    lepus::Context* context,
    fml::RefPtr<tasm::ResponseHandlerInLepus>& handler) {
  return Utils::CreateResponseHandler(context, lepus::Value(handler));
}

ResponseHandlerInLepus*
ResponseHandlerInLepus::GetResponseHandlerFromLepusValue(
    const lepus::Value& binding_object) {
  if (!binding_object.IsObject()) {
    return nullptr;
  }

  auto context_proxy_property = binding_object.GetProperty(
      BASE_STATIC_STRING(runtime::kInnerRuntimeProxy));
  if (!context_proxy_property.IsRefCounted()) {
    return nullptr;
  }

  ResponseHandlerInLepus* handler_proxy =
      fml::static_ref_ptr_cast<ResponseHandlerInLepus>(
          context_proxy_property.RefCounted())
          .get();
  return handler_proxy;
}

void ResponseHandlerInLepus::AddResourceListener(
    base::MoveOnlyClosure<void, tasm::BundleResourceInfo> closure) {
  promise_->AddCallback([delegate = &delegate_, closure = std::move(closure)](
                            tasm::BundleResourceInfo bundle_info) mutable {
    // it's safe here because callback won't be invoked if engine destroyed
    // it's guaranteed by LazyBundleLoader.
    delegate->InvokeResponsePromiseCallback(
        [bundle_info = std::move(bundle_info),
         closure = std::move(closure)]() mutable {
          closure(std::move(bundle_info));
        });
  });
}

}  // namespace tasm
}  // namespace lynx
