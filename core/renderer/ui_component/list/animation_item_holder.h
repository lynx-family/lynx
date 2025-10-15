// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_ANIMATION_ITEM_HOLDER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_ANIMATION_ITEM_HOLDER_H_

#include <limits>
#include <string>

#include "core/renderer/ui_component/list/item_holder.h"
#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {

class AnimationItemHolder final : public ItemHolder {
 public:
  AnimationItemHolder(int index, const std::string& item_key,
                      AnimationDelegate* delegate);

  void UpdateLayoutToPlatform(float content_size, float container_size,
                              Element* element) override;
  void UpdateLayoutFromManager(float left, float top) override;

  void DoAnimationFrame(float progress) override;
  void EndAnimation() override;
  void RecycleAfterAnimation(list::ItemHolderAnimationType type) override;
  void MarkInsertOpacity() override;

 private:
  float GetRTLLeft(float content_size, float container_size, float left,
                   float width) const;
  AnimationDelegate* animation_delegate_;
  float animation_origin_left_{std::numeric_limits<float>::quiet_NaN()};
  float animation_origin_top_{std::numeric_limits<float>::quiet_NaN()};
  // 1.f means the opacity animation of 1 -> 0, and the 0.f is the opposite.
  float animation_origin_opacity_{std::numeric_limits<float>::quiet_NaN()};
  float content_size_{std::numeric_limits<float>::quiet_NaN()};
  float container_width_{std::numeric_limits<float>::quiet_NaN()};
  list::ItemHolderAnimationType animation_type_{
      list::ItemHolderAnimationType::kNone};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_ANIMATION_ITEM_HOLDER_H_
