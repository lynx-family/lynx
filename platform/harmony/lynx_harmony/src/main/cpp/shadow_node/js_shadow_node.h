// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_JS_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_JS_SHADOW_NODE_H_

#include <node_api.h>

#include <shared_mutex>
#include <string>

#include "base/include/fml/memory/ref_counted.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

class JSExtraBundle : fml::RefCountedThreadSafeStorage {
 public:
  explicit JSExtraBundle(napi_env env, napi_value js_object);
  // Released in UpdateExtraBundle on main thread!.
  ~JSExtraBundle() override;
  napi_value JSObject() const;

 protected:
  void ReleaseSelf() const override;

 private:
  napi_env env_{nullptr};
  napi_ref js_object_{nullptr};
};

class JSShadowNode : public ShadowNode, public CustomMeasureFunc {
 public:
  JSShadowNode(const JSShadowNode&) = delete;
  JSShadowNode(JSShadowNode&&) = delete;
  JSShadowNode& operator=(const JSShadowNode&) = delete;
  JSShadowNode& operator=(JSShadowNode&&) = delete;
  JSShadowNode(const int sign, const std::string& tag, napi_env env)
      : ShadowNode(sign, tag), env_(env) {}
  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void Align() override;
  void UpdateProps(PropBundleHarmony* props) override;
  void OnPropsUpdate(const char* attr, const lepus::Value& value) override;
  static napi_value Init(napi_env env, napi_value exports);
  napi_value GetJSObject() const override;
  bool HasJSObject() const override { return true; }
  fml::RefPtr<fml::RefCountedThreadSafeStorage> getExtraBundle() override;
  void Destroy() override;
  void MarkDestroyed() override;
  void MarkDirty() const override;
  void RequestLayout() const override;

 private:
  static napi_value SetExtraDataFunc(napi_env env, napi_callback_info info);
  static napi_value SetMeasureFunc(napi_env env, napi_callback_info info);
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value MarkDirty(napi_env env, napi_callback_info info);
  static napi_value RequestLayout(napi_env env, napi_callback_info info);
  static napi_value GetChildren(napi_env, napi_callback_info info);
  mutable std::shared_mutex destroyed_mutex_;
  napi_ref measure_{nullptr};
  napi_ref align_{nullptr};
  napi_ref extra_bundle_getter_{nullptr};
  napi_ref js_ref_{nullptr};
  napi_env env_{nullptr};
  ~JSShadowNode() override;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_JS_SHADOW_NODE_H_
