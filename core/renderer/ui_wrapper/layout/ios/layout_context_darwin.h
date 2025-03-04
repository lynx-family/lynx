// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_IOS_LAYOUT_CONTEXT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_IOS_LAYOUT_CONTEXT_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <string>
#include <unordered_set>

#import "LynxShadowNodeOwner.h"
#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"

namespace lynx {
namespace tasm {
class LayoutContextDarwin : public LayoutCtxPlatformImpl {
 public:
  LayoutContextDarwin(LynxShadowNodeOwner* owner);
  ~LayoutContextDarwin() override;

  int CreateLayoutNode(int sign, const std::string& tag, PropBundle* props,
                       bool allow_inline) override;
  void UpdateLayoutNode(int sign, PropBundle* props) override;
  void InsertLayoutNode(int parent, int child, int index) override;
  void RemoveLayoutNode(int parent, int child, int index) override;
  void MoveLayoutNode(int parent, int child, int from_index,
                      int to_index) override;
  void DestroyLayoutNodes(const std::unordered_set<int>& ids) override;
  void ScheduleLayout(base::closure) override;
  void OnLayoutBefore(int sign) override;
  void OnLayout(int sign, float left, float top, float width, float height,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override;
  void Destroy() override;
  void SetFontFaces(const FontFacesMap& fontFaces) override;
  void UpdateRootSize(float width, float height) override;
  std::unique_ptr<PlatformExtraBundle> GetPlatformExtraBundle(
      int32_t signature) override;
  std::unique_ptr<PlatformExtraBundleHolder> ReleasePlatformBundleHolder()
      override;
  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) override;

 private:
  LynxShadowNodeOwner* nodeOwner;

  LayoutContextDarwin(const LayoutContextDarwin&) = delete;
  LayoutContextDarwin& operator=(const LayoutContextDarwin&) = delete;
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_IOS_LAYOUT_CONTEXT_DARWIN_H_
