// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/native_node_content.h"

#include <arkui/native_node_napi.h>
#include <node_api.h>

#include "base/include/platform/harmony/napi_util.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

napi_value NativeNodeContent::Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      {"content", nullptr, nullptr, GetNodeContent, nullptr, nullptr,
       napi_default_jsproperty, nullptr},
      {"uiTagName", nullptr, nullptr, GetUITagName, nullptr, nullptr,
       napi_default_jsproperty, nullptr},
  };
  napi_value cons;
  napi_define_class(env, "NativeContent", NAPI_AUTO_LENGTH, Constructor,
                    nullptr, sizeof(properties) / sizeof(properties[0]),
                    properties, &cons);
  napi_set_named_property(env, exports, "NativeContent", cons);
  return exports;
}

napi_value NativeNodeContent::GetNodeContent(napi_env env,
                                             napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  NativeNodeContent* node{nullptr};
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node));
  if (node) {
    return node->JSNodeContent();
  }
  return nullptr;
}

napi_value NativeNodeContent::GetUITagName(napi_env env,
                                           napi_callback_info info) {
  napi_value undefined{nullptr};
  if (napi_get_undefined(env, &undefined) != napi_ok) {
    return nullptr;
  }
  napi_value js_this{nullptr};
  size_t argc = 0;
  if (napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr) !=
      napi_ok) {
    return undefined;
  }
  NativeNodeContent* native_node_content{nullptr};
  if (napi_unwrap(env, js_this,
                  reinterpret_cast<void**>(&native_node_content)) != napi_ok ||
      !native_node_content || !native_node_content->UI()) {
    return undefined;
  }
  const auto& tag = native_node_content->UI()->Tag();
  napi_value result{nullptr};
  if (napi_create_string_utf8(env, tag.c_str(), tag.size(), &result) !=
      napi_ok) {
    return undefined;
  }
  return result;
}

napi_value NativeNodeContent::Constructor(napi_env env,
                                          napi_callback_info info) {
  napi_value js_this;
  size_t argc{1};
  /**
   * 0 - NodeContent from arkui.
   */
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  NativeNodeContent* content = new NativeNodeContent();
  napi_wrap(
      env, js_this, reinterpret_cast<void*>(content),
      [](napi_env env, void* data, void* hint) {
        // Do nothing, this object is managed by UIBase via unique_ptr.
      },
      nullptr, nullptr);
  content->env_ = env;
  napi_create_reference(env, js_this, 1, &content->js_this_);
  napi_create_reference(env, argv[0], 1, &content->js_node_content_);
  OH_ArkUI_GetNodeContentFromNapiValue(env, argv[0], &content->node_content_);
  return js_this;
}

NativeNodeContent::~NativeNodeContent() {
  base::NapiHandleScope scope(env_);
  napi_value js_this = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  NativeNodeContent* content;
  napi_remove_wrap(env_, js_this, reinterpret_cast<void**>(&content));
  napi_delete_reference(env_, js_this_);
  napi_delete_reference(env_, js_node_content_);
}

napi_value NativeNodeContent::JSNodeContent() const {
  return base::NapiUtil::GetReferenceNapiValue(env_, js_node_content_);
}

napi_value NativeNodeContent::JSObject() const {
  return base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
