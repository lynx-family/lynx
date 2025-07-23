// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/resource/response_handler_in_lepus.h"

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/common/resource/response_promise.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"

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
  lepus::Value proxy_binding = lepus::LEPUSValueHelper::CreateObject(context);
  proxy_binding.SetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy),
                            lepus::Value(handler));
  if (context->IsVMContext()) {
#if !ENABLE_JUST_LEPUSNG
    Utils::RegisterMethodToResponseHandler(context, proxy_binding);
#endif
  } else {
    Utils::RegisterNGMethodToResponseHandler(context, proxy_binding);
  }
  return proxy_binding;
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

}  // namespace tasm
}  // namespace lynx
