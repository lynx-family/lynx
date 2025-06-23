// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_SHADOW_NODE_OWNER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_SHADOW_NODE_OWNER_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "core/public/layout_node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_prop_bundle_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ShadowNodeOwner;
class PubLynxContext;

class BASE_EXPORT PubShadowNodeOwner {
 public:
  explicit PubShadowNodeOwner(napi_env env, napi_value shadow_node_owner);
  ~PubShadowNodeOwner() = default;
  int CreateShadowNode(int sign, const std::string& tag,
                       PubPropBundleHarmony* painting_data,
                       bool allow_inline) const;
  void RemoveLayoutNode(int parent, int child, int index) const;
  void InsertLayoutNode(int parent, int child, int index) const;
  void MoveLayoutNode(int parent, int child, int from_index,
                      int to_index) const;
  void DestroyNode(int sign) const;
  void Destroy() const;
  void OnLayoutBefore(int id) const;
  void OnLayout(int id, float left, float top, float width, float height) const;
  fml::RefPtr<fml::RefCountedThreadSafeStorage> GetExtraBundle(int id) const;
  void UpdateLayoutNode(int id, PubPropBundleHarmony* painting_data) const;
  void SetLayoutNodeManager(tasm::LayoutNodeManager* layout_node_manager) const;
  void ScheduleLayout(base::closure callback) const;
  void SetContext(PubLynxContext* context);
  LayoutResult MeasureNode(int id, float width, int32_t width_mode,
                           float height, int32_t height_mode,
                           bool final_measure);
  ShadowNodeOwner* NodeOwner() const { return shadow_node_owner_.get(); };

 private:
  std::shared_ptr<ShadowNodeOwner> shadow_node_owner_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_SHADOW_NODE_OWNER_H_
