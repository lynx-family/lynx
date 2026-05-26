// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <napi/native_api.h>

#include <string>

#include "base/include/log/logging.h"
#include "core/base/lynx_export.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/shadow_node/markdown_shadow_node.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/ui/ui_markdown.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

UIBase* CreateMarkdownUI(LynxContext* context, int sign,
                         const std::string& tag) {
  LOGI("XElement markdown UI creator called, sign: " << sign
                                                     << ", tag: " << tag);
  return UIMarkdown::Make(context, sign, tag);
}

ShadowNode* CreateMarkdownShadowNode(int sign, const std::string& tag) {
  LOGI("XElement markdown shadow creator called, sign: " << sign
                                                         << ", tag: " << tag);
  return MarkdownShadowNode::Make(sign, tag);
}

napi_value InitMarkdown(napi_env env, napi_callback_info info) {
  LOGI("XElement markdown initMarkdown called");
  auto& map = LynxContext::GetCAPINodeInfoMap();
  map["markdown"] = {CreateMarkdownUI, CreateMarkdownShadowNode,
                     LayoutNodeType::CUSTOM};
  napi_value undefined_value;
  napi_get_undefined(env, &undefined_value);
  return undefined_value;
}

}  // namespace

extern "C" {

LYNX_EXPORT UIBase* UILynxMarkdownCreateFunc(LynxContext* context, int sign,
                                             const std::string& tag) {
  return CreateMarkdownUI(context, sign, tag);
}

LYNX_EXPORT ShadowNode* UILynxMarkdownShadowNodeCreateFunc(
    int sign, const std::string& tag) {
  return CreateMarkdownShadowNode(sign, tag);
}

LYNX_EXPORT napi_value Init(napi_env env, napi_value exports) {
  LOGI("XElement markdown native module Init");
  napi_value init_markdown;
  napi_create_function(env, "initMarkdown", NAPI_AUTO_LENGTH, InitMarkdown,
                       nullptr, &init_markdown);
  napi_set_named_property(env, exports, "initMarkdown", init_markdown);
  return exports;
}

}  // extern "C"

static napi_module lynx_xelement_markdown_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "lynx_xelement_markdown",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  LOGI("Register XElement markdown native module");
  napi_module_register(&lynx_xelement_markdown_module);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
