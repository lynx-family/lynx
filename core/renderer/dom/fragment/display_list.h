// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_H_

#include <cstdint>
#include <type_traits>

#include "base/include/auto_create_optional.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

// Wrapper struct for operation data that provides lazy allocation
struct OpData {
  base::InlineVector<int32_t, 8> ops;
  base::InlineVector<int32_t, 16> int_data;
  base::InlineVector<float, 16> float_data;
};

enum class DisplayListOpType : int32_t {
  kBegin = 0,
  kEnd = 1,
  kFill = 2,
  kDrawView = 3,
  kText = 6,
  kImage = 7,
  kCustom = 8,
  kBorder = 9,
  kClipRect = 10,
  kRecordBox = 11,
  kLinearGradient = 12,
};

enum class DisplayListSubtreePropertyOpType : int32_t {
  kTransform = 0,
  kOpacity = 1,
};

enum class DisplayListOpCategory : int8_t {
  kContent = 0,
  kSubtreeProperty = 1,
};

template <DisplayListOpCategory category>
struct DisplayListOpCategoryTraits {
  using OpType = DisplayListOpType;
};

template <>
struct DisplayListOpCategoryTraits<DisplayListOpCategory::kSubtreeProperty> {
  using OpType = DisplayListSubtreePropertyOpType;
};

/**
 * @brief DisplayList is a data structure that stores the display list of a
 * fragment.
 *
 * The DisplayList separates content operations from subtree-influencing group
 * properties (transform) to optimize animation performance. Group
 * properties affect the entire subtree and only apply to owner layers, making
 * them special operations that can be updated independently.
 *
 * Data Layout:
 * - content_ops_: Content operations (Fill, DrawView, Text, Image, Custom,
 * Begin, End)
 * - content_int_data_: Integer parameters for content operations
 * - content_float_data_: Float parameters for content operations
 * - subtree_property_ops_: Subtree-influencing group properties (Transform)
 * - subtree_property_int_data_: Integer parameters for subtree properties
 * - subtree_property_float_data_: Float parameters for subtree properties
 *
 * For each operation, the *_int_data_ stores the parameter counts of both int
 * params and float params. Then the actual parameters are appended in sequence.
 */

extern "C" {
typedef struct SubtreeProperty {
  DisplayListSubtreePropertyOpType type;
  union Data {
    float transform[16];
    float opacity;
  } data;
} SubtreeProperty;

// Once the size and offset changed, should change all related buffer based
// operations. (e.g. On android we use this in DirectByteBuffer to sync data.)
static_assert(sizeof(SubtreeProperty) == 68,
              "SubtreeProperty size must be 68 bytes (4 + 64)");
static_assert(offsetof(SubtreeProperty, type) == 0,
              "type field must be at offset 0");
static_assert(offsetof(SubtreeProperty, data) == 4,
              "data field must be at offset 4");
static_assert(offsetof(SubtreeProperty, data.transform) == 4,
              "transform array must start at offset 4");
static_assert(offsetof(SubtreeProperty, data.opacity) == 4,
              "opacity must be at offset 4 (union shared)");
static_assert(std::is_standard_layout<SubtreeProperty>::value,
              "SubtreeProperty must be standard layout for JNI");
static_assert(
    std::is_trivially_copyable<SubtreeProperty>::value,
    "SubtreeProperty must be trivially copyable for DirectByteBuffer");
}

class DisplayList {
 public:
  DisplayList() = default;
  DisplayList(float dx, float dy) : render_offset_{dx, dy} {}
  ~DisplayList() = default;

  // Once the display list is built up by display list builder, it should be
  // move only.
  DisplayList(const DisplayList&) = delete;
  DisplayList& operator=(const DisplayList&) = delete;

  DisplayList(DisplayList&&) = default;
  DisplayList& operator=(DisplayList&&) = default;

  void Reserve(int32_t capacity);

  // Direct array access for JNI
  const int32_t* GetContentOpTypesData() const {
    return content_data_.has_value() ? content_data_->ops.data() : nullptr;
  }
  const int32_t* GetContentIntData() const {
    return content_data_.has_value() ? content_data_->int_data.data() : nullptr;
  }
  const float* GetContentFloatData() const {
    return content_data_.has_value() ? content_data_->float_data.data()
                                     : nullptr;
  }

  const float* GetRenderOffset() const { return render_offset_; }

  // Size accessors
  size_t GetContentOpTypesSize() const {
    return content_data_.has_value() ? content_data_->ops.size() : 0;
  }
  bool HasContent() const { return GetContentOpTypesSize() != 0; }
  size_t GetContentIntDataSize() const {
    return content_data_.has_value() ? content_data_->int_data.size() : 0;
  }
  size_t GetContentFloatDataSize() const {
    return content_data_.has_value() ? content_data_->float_data.size() : 0;
  }

  int32_t GetOpAtIndex(size_t index) const { return content_data_->ops[index]; }

  int32_t GetIntAtIndex(size_t index) const {
    return content_data_->int_data[index];
  }

  float GetFloatAtIndex(size_t index) const {
    return content_data_->float_data[index];
  }

  size_t GetSubtreePropertiesSize() const {
    return subtree_properties_.has_value() ? subtree_properties_->size() : 0;
  }

  const SubtreeProperty* GetSubtreePropertiesData() const {
    return subtree_properties_.has_value() ? subtree_properties_->data()
                                           : nullptr;
  }

  void MarkRootNeedClipBounds() { root_need_clip_bounds_ = true; }

  bool RootNeedClipBounds() const { return root_need_clip_bounds_; }

  void Clear();

  void ClearSubtreeProperties();

  void AddLinearGradient(float angle, const base::Vector<uint32_t>& colors,
                         const base::Vector<float>& stops, int32_t tiling_index,
                         int32_t clip_index, int32_t repeat_x,
                         int32_t repeat_y);

  template <typename... Args>
  auto AddOperation(DisplayListOpType type, Args... args) {
    AddOperationToData(content_data_, type, args...);
  }

  void AddSubLayer(int id) { sub_layers_.emplace_back(id); }
  const auto& SubLayers() const { return sub_layers_; }

  void AddSubtreeProperty(const SubtreeProperty& prop) {
    subtree_properties_->push_back(prop);
  }

 private:
  template <typename OpType, typename... Args>
  void AddOperationToData(base::auto_create_optional<OpData>& data_store,
                          OpType type, Args... args);

 private:
  // Content operations (stable during animations) - lazy allocated
  base::auto_create_optional<OpData> content_data_;

  // Subtree-influencing group properties (frequently updated during animations)
  // These operations affect the entire subtree and only apply to owner layers -
  // lazy allocated
  base::auto_create_optional<base::InlineVector<SubtreeProperty, 1>>
      subtree_properties_;

  // Platform renderers that belongs to the layer holds this displayList. Used
  // for re-construct ot update the platform renderer hierachy.
  base::InlineVector<int, 16> sub_layers_;

  float render_offset_[2] = {0, 0};

  bool root_need_clip_bounds_{false};
};

template <typename OpType, typename... Args>
void DisplayList::AddOperationToData(
    base::auto_create_optional<OpData>& data_store, OpType type, Args... args) {
  static_assert((... && (std::is_same_v<std::decay_t<Args>, int32_t> ||
                         std::is_same_v<std::decay_t<Args>, float>)),
                "AddOperation only accepts int32_t and float parameters");

  OpData* op_data =
      &(*data_store);  // auto_create_optional creates on first access

  op_data->ops.push_back(static_cast<int32_t>(type));

  // Handle empty parameter pack case
  if constexpr (sizeof...(Args) == 0) {
    // No parameters - just store counts [0, 0]
    op_data->int_data.push_back(0);
    op_data->int_data.push_back(0);
  } else {
    // Pre-calculate sizes to avoid multiple reallocations
    constexpr size_t int_count =
        (... + (std::is_same_v<std::decay_t<Args>, int32_t> ? 1 : 0));
    constexpr size_t float_count =
        (... + (std::is_same_v<std::decay_t<Args>, float> ? 1 : 0));

    // Pre-calculate and reserve space to avoid multiple reallocations
    op_data->int_data.reserve(op_data->int_data.size() + 2 + int_count);
    if constexpr (float_count > 0) {
      op_data->float_data.reserve(op_data->float_data.size() + float_count);
    }

    // Store counts
    op_data->int_data.push_back(static_cast<int32_t>(int_count));
    op_data->int_data.push_back(static_cast<int32_t>(float_count));

    // Store parameters using fold expression - no runtime type checking needed
    ((std::is_same_v<std::decay_t<Args>, int32_t>
          ? op_data->int_data.push_back(static_cast<int32_t>(args))
          : op_data->float_data.push_back(static_cast<float>(args))),
     ...);
  }
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_H_
