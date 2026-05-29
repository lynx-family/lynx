// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SEMANTICS_SEMANTICS_NODE_H_
#define CLAY_UI_SEMANTICS_SEMANTICS_NODE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/ui/rendering/abstract_node.h"
#include "clay/ui/semantics/semantics_update_builder.h"

namespace clay {

class SemanticsOwner;
class BaseView;

class SemanticsNode : public AbstractNode,
                      public fml::RefCountedThreadSafe<SemanticsNode> {
 public:
  enum class SemanticsAction : int32_t {
    kTap = 1 << 0,
    kLongPress = 1 << 1,
    kScrollLeft = 1 << 2,
    kScrollRight = 1 << 3,
    kScrollUp = 1 << 4,
    kScrollDown = 1 << 5,
    kShowOnScreen = 1 << 6,
  };

  enum class SemanticsFlag : int32_t {
    kHasImplicitScrolling = 1 << 0,
  };

  SemanticsNode(SemanticsOwner* owner, BaseView* view, int id);
  SemanticsNode(BaseView* view, int id);
  ~SemanticsNode() override;
  struct SemanticsData {
    SemanticsData() = default;

    int32_t actions;
    int32_t flags;
    int32_t scroll_children;
    float scroll_position = 0.f;
    float scroll_extent_max = 0.f;
    float scroll_extent_min = 0.f;
    std::string id_selector;
    std::u16string label;
    FloatRect semantics_bounds;
    Transform transform;
    std::vector<std::string> accessibility_elements;

    bool operator==(const SemanticsData& other) {
      return actions == other.actions && flags == other.flags &&
             scroll_children == other.scroll_children &&
             scroll_position == other.scroll_position &&
             scroll_extent_max == other.scroll_extent_max &&
             scroll_extent_min == other.scroll_extent_min &&
             id_selector == other.id_selector && label == other.label &&
             semantics_bounds == other.semantics_bounds &&
             transform == other.transform &&
             accessibility_elements == other.accessibility_elements;
    }

    bool operator!=(const SemanticsData& other) { return !(*this == other); }
  };

  void Attach(SemanticsOwner* owner);
  void Detach();
  bool Attached() const { return owner_ != nullptr; }
  bool IsDirty() const { return dirty_; }

  void AddToUpdate(SemanticsUpdateBuilder* builder);

  int Id() const { return id_; }

  SemanticsData& GetSemanticsData() { return data_; }

  SemanticsOwner* Owner() const { return owner_; }

  void UpdateWith(const std::vector<fml::RefPtr<SemanticsNode>>& new_children,
                  bool need_check_children, bool force_update = false);

  void RedepthChildren() override;
  void MarkDirty();

  // This should happen before updating data.
  void TransferCurrentData() { old_data_ = data_; }

  const std::vector<fml::RefPtr<SemanticsNode>>& GetChildren() const {
    return children_;
  }

  BaseView* OwnerView() const { return owner_view_; }

  std::u16string GetAccessibilityLabelWithChildren() const;

 private:
  int32_t id_;
  // Contains the children in inverse hit test order (i.e. paint order).
  std::vector<fml::RefPtr<SemanticsNode>> children_;
  SemanticsOwner* owner_ = nullptr;
  bool dirty_ = false;

  // Help to replace children to check if need MarkDirty.
  bool dead_ = false;

  BaseView* owner_view_ = nullptr;

  // Basic data that Semantics Node must have.
  SemanticsData data_;
  SemanticsData old_data_;
};
}  // namespace clay

#endif  // CLAY_UI_SEMANTICS_SEMANTICS_NODE_H_
