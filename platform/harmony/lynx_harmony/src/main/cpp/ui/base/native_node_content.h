// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_NATIVE_NODE_CONTENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_NATIVE_NODE_CONTENT_H_
#include <arkui/native_type.h>
#include <node_api.h>
namespace lynx::tasm::harmony {
class UIBase;
}
namespace lynx {
namespace tasm {
namespace harmony {

class NativeNodeContent {
 public:
  static napi_value Init(napi_env env, napi_value exports);
  ArkUI_NodeContentHandle NodeContentHandle() const { return node_content_; }
  ~NativeNodeContent();
  napi_value JSNodeContent() const;
  napi_value JSObject() const;
  void SetUI(UIBase* ui) { ui_ = ui; }
  UIBase* UI() const { return ui_; }

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value GetNodeContent(napi_env env, napi_callback_info info);
  napi_env env_{nullptr};
  napi_ref js_this_{nullptr};
  napi_ref js_node_content_{nullptr};
  ArkUI_NodeContentHandle node_content_{nullptr};
  UIBase* ui_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_NATIVE_NODE_CONTENT_H_
