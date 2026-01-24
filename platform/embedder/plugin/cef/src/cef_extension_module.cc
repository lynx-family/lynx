// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/plugin/cef/src/cef_extension_module.h"

#include "platform/embedder/plugin/cef/src/cef_webview.h"
#include "platform/embedder/plugin/cef/src/cef_webview_constants.h"
#include "platform/embedder/public/capi/lynx_log_capi.h"
#include "platform/embedder/public/capi/lynx_trace_capi.h"

static constexpr const char* TRACE_NAME_REGISTER_VIEW =
    "lynx_view_register_cef_webview";
static constexpr const char* TRACE_NAME_CREATE_MODULE =
    "cef_extension_module_create_extension_module";

namespace lynx {
namespace plugin {
namespace embedder {

void CEFExtensionModule::OnLynxViewCreate(lynx_view_t* lynx_view) {
  LYNX_CAPI_TRACE_BEGIN(CEF_TRACE_CATEGORY, TRACE_NAME_REGISTER_VIEW);
  lynx_view_register_native_view(lynx_view, "x-webview",
                                 &cef_webview_create_view, lynx_view);
  LYNX_CAPI_TRACE_END(CEF_TRACE_CATEGORY, TRACE_NAME_REGISTER_VIEW);
}

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

LYNX_EXTERN_C lynx_extension_module_t*
cef_extension_module_create_extension_module(void* opaque) {
  LYNX_CAPI_LOG(LYNX_LOG_INFO, LOG_TAG, "Create CEFExtensionModule");
  LYNX_CAPI_TRACE_BEGIN(CEF_TRACE_CATEGORY, TRACE_NAME_CREATE_MODULE);
  auto* module = new lynx::plugin::embedder::CEFExtensionModule();
  lynx_extension_module_t* c_module =
      lynx_extension_module_create_with_finalizer(
          module, [](lynx_extension_module_t* m, void* user_data) {
            if (user_data) {
              LYNX_CAPI_LOG(LYNX_LOG_INFO, LOG_TAG,
                            "Delete CEFExtensionModule");
              delete reinterpret_cast<
                  lynx::plugin::embedder::CEFExtensionModule*>(user_data);
            }
          });

  module->SetCModule(c_module);
  LYNX_CAPI_TRACE_END(CEF_TRACE_CATEGORY, TRACE_NAME_CREATE_MODULE);
  return c_module;
}
