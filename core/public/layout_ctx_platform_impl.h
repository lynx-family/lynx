// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LAYOUT_CTX_PLATFORM_IMPL_H_
#define CORE_PUBLIC_LAYOUT_CTX_PLATFORM_IMPL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "core/public/layout_node_manager.h"
#include "core/public/platform_extra_bundle.h"
#include "core/public/prop_bundle.h"

namespace lynx {
namespace shell {
class LynxShell;
}  // namespace shell
namespace tasm {

using FontFaceAttrsMap = std::unordered_map<std::string, std::string>;

using FontFaceToken = std::pair<std::string, FontFaceAttrsMap>;

using FontFacesMap =
    std::unordered_map<std::string,
                       std::vector<std::shared_ptr<FontFaceToken>>>;

class LayoutCtxPlatformImpl {
 public:
  virtual ~LayoutCtxPlatformImpl() = default;
  virtual int CreateLayoutNode(int id, const std::string& tag,
                               PropBundle* props, bool allow_inline) = 0;
  virtual void UpdateLayoutNode(int id, PropBundle* props) = 0;
  virtual void InsertLayoutNode(int parent, int child, int index) = 0;
  virtual void RemoveLayoutNode(int parent, int child, int index) = 0;
  virtual void MoveLayoutNode(int parent, int child, int from_index,
                              int to_index) = 0;
  virtual void DestroyLayoutNodes(const std::unordered_set<int>& ids) = 0;
  virtual void ScheduleLayout(base::closure callback) = 0;
  virtual void OnLayoutBefore(int id) = 0;
  virtual void OnLayout(int id, float left, float top, float width,
                        float height, const std::array<float, 4>& paddings,
                        const std::array<float, 4>& borders) = 0;
  virtual void Destroy() = 0;

  virtual void SetFontFaces(const FontFacesMap& fontfaces) = 0;
  virtual void SetLynxShell(shell::LynxShell* shell) {}
  virtual void UpdateRootSize(float width, float height) {}
  virtual std::unique_ptr<PlatformExtraBundle> GetPlatformExtraBundle(
      int32_t id) {
    return std::unique_ptr<PlatformExtraBundle>();
  }
  virtual void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) = 0;
  virtual std::unique_ptr<PlatformExtraBundleHolder>
  ReleasePlatformBundleHolder() {
    return std::unique_ptr<PlatformExtraBundleHolder>();
  }
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_PUBLIC_LAYOUT_CTX_PLATFORM_IMPL_H_
