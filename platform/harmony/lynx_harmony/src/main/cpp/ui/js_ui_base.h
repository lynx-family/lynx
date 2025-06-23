// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_JS_UI_BASE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_JS_UI_BASE_H_

#include <node_api.h>

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

using UIMethodCallback =
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>;

struct JSCallbackWrapper {
  UIMethodCallback callback;
};

class JSUIBase : public UIBase {
 public:
  static napi_value Init(napi_env env, napi_value exports);

  void AddChild(UIBase* child, int index) override;
  void UpdateProps(PropBundleHarmony* props) override;
  void UpdateLayout(float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnNodeReady() override;
  void RemoveChild(UIBase* child) override;
  bool Focusable() override;
  void OnFocusChange(bool has_focus, bool is_focus_transition) override;
  JSUIBase(LynxContext* context, ArkUI_NodeHandle node, int sign,
           const std::string& tag, bool has_customized_layout)
      : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag, has_customized_layout) {
    if (node) {
      frame_node_ = node;
      NodeManager::Instance().InsertNode(node_, node, 0);
    }
  }

  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    UIMethodCallback callback) override;

  void SetFrameNode(ArkUI_NodeHandle frame_node);
  bool HasJSObject() override { return true; }
  void UpdateExtraData(
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) override;

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value GetChildren(napi_env env, napi_callback_info info);
  static napi_value SetFrameNode(napi_env env, napi_callback_info info);
  static napi_value SetFocusedUI(napi_env env, napi_callback_info info);
  static napi_value UnsetFocusedUI(napi_env env, napi_callback_info info);
  static napi_value SetChildrenManagementFuncs(napi_env env,
                                               napi_callback_info);

  static napi_value GetUIFromNativeContent(napi_env env,
                                           napi_callback_info info);
  static napi_value GetContentSize(napi_env env, napi_callback_info info);

  ~JSUIBase() override;

  napi_env env_{nullptr};
  napi_ref js_ref_{nullptr};
  napi_ref js_update_{nullptr};
  napi_ref js_layout_{nullptr};
  napi_ref js_node_ready_{nullptr};
  napi_ref js_ui_method_{nullptr};
  napi_ref js_dispose_{nullptr};
  napi_ref js_focus_change_{nullptr};
  napi_ref js_focusable_{nullptr};
  napi_ref js_insert_child_{nullptr};
  napi_ref js_remove_child_{nullptr};
  napi_ref js_update_extra_data_{nullptr};
  ArkUI_NodeHandle frame_node_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_JS_UI_BASE_H_
