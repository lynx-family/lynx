// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_UI_OWNER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_UI_OWNER_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "base/include/fml/memory/ref_counted.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_prop_bundle_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class UIOwner;
class PubLynxContext;
class BASE_EXPORT PubUIOwner {
 public:
  PubUIOwner(napi_env env, napi_value ui_owner);
  ~PubUIOwner() = default;
  void CreateUI(int sign, const std::string& tag,
                PubPropBundleHarmony* painting_data, uint32_t node_index) const;
  void InsertUI(int parent, int child, int index) const;
  void RemoveUI(int parent, int child, int index, bool is_move) const;
  void UpdateUI(int sign, PubPropBundleHarmony* props) const;
  void DestroyUI(int parent, int child, int index) const;
  void OnNodeReady(int sign) const;
  void UpdateExtraData(
      int sign,
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) const;
  void UpdateLayout(int sign, float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) const;
  void OnLayoutFinish(int32_t component_id, int64_t operation_id) const;
  void AttachPageRoot(napi_env env, napi_value root_content) const;
  UIOwner* Owner() const { return ui_owner_.get(); }
  void SetContext(PubLynxContext* context) const;
  void InvokeUIMethod(
      int32_t id, const std::string& method, const pub::Value& args,
      base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) const;
  int IndexOf(int child_id) const;
  int GetUINodeByPosition(float x, float y) const;
  std::string GetTag(int sign) const;

 private:
  std::shared_ptr<UIOwner> ui_owner_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_UI_OWNER_H_
