// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_ANIMATION_ITEM_HOLDER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_ANIMATION_ITEM_HOLDER_H_

#include <array>
#include <string>

#include "core/renderer/ui_component/list/item_holder.h"

namespace lynx {
namespace tasm {

class AnimationItemHolder : public ItemHolder {
 public:
  AnimationItemHolder(int index, const std::string& item_key);
  void DoAnimationFrame(float progress) override;
  void UpdateLayoutToPlatform(float content_size, float container_size,
                              Element* element) override;
  void UpdateLayoutFromManager(float left, float top) override;
  void SetAnimationDelegate(ItemHolder::AnimationDelegate* delegate) override;
  void EndAnimation() override;

 private:
  float GetRTLLeft(float content_size, float container_size, float left,
                   float width);
  AnimationDelegate* animation_delegate_;
  float animation_origin_left_{-1.f};
  float animation_origin_top_{-1.f};
  float content_size_{0.f};
  float container_size_{0.f};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_ANIMATION_ITEM_HOLDER_H_
