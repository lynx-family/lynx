// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "base/include/auto_create_optional.h"
#include "base/include/vector.h"

namespace lynx {
namespace tasm {

struct OpData {
  base::InlineVector<int32_t, 8> ops;
  base::InlineVector<int32_t, 16> int_data;
  base::InlineVector<float, 16> float_data;
};

// NOTE: This enum is serialized into the cross-platform DisplayList protocol.
// Any addition/renumbering MUST be synchronized with all consumers
// (C++/Android/iOS/etc.). Consumers MUST tolerate unknown op types by
// skipping them based on per-op int/float parameter counts.
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
  kBoxShadow = 13,
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
 * - content_items_: Array of DisplayListItem structs (fixed-size, tagged union)
 * - content_data_: Trailing variable-length data region (gradient colors/stops)
 * - subtree_properties_: Subtree-influencing group properties (Transform)
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

typedef struct DisplayListItem {
  DisplayListOpType type;  // 4 bytes, offset 0
  union Payload {
    struct {
      int32_t id;
      int32_t type;
      float x;
      float y;
      float w;
      float h;
    } begin;
    struct {
      uint32_t color;
      int32_t clip_index;
    } fill;
    struct {
      int32_t view_id;
    } draw_view;
    struct {
      int32_t text_id;
      int32_t box_index;
    } text;
    struct {
      int32_t image_id;
      int32_t box_index;
    } image;
    struct {
      int32_t out_index;
      int32_t inner_index;
      uint32_t colors[4];
      int32_t styles[4];
    } border;
    struct {
      float x;
      float y;
      float w;
      float h;
      float radii[8];
      uint32_t has_radii;
    } record_box;
    struct {
      float x;
      float y;
      float w;
      float h;
      float radii[8];
      uint32_t has_radii;
    } clip_rect;
    struct {
      uint32_t color_count_offset;
      uint32_t color_count;
      uint32_t stop_count_offset;
      uint32_t stop_count;
      int32_t tiling_index;
      int32_t clip_index;
      int32_t repeat_x;
      int32_t repeat_y;
      float angle;
    } linear_gradient;
    struct {
      int32_t shadow_box_index;
      int32_t clip_box_index;
      uint32_t color;
      float blur_radius;
      int32_t clip_mode;
    } box_shadow;
  } payload;
} DisplayListItem;

// ABI locking: static_asserts for every field that cross-platform consumers
// read
static_assert(sizeof(DisplayListItem) == 56,
              "DisplayListItem size must be 56 bytes");
static_assert(offsetof(DisplayListItem, type) == 0,
              "type field must be at offset 0");
static_assert(offsetof(DisplayListItem, payload) == 4,
              "payload union must be at offset 4");

// Begin payload offsets
static_assert(offsetof(DisplayListItem, payload.begin.id) == 4,
              "begin.id must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.begin.type) == 8,
              "begin.type must be at offset 8");
static_assert(offsetof(DisplayListItem, payload.begin.x) == 12,
              "begin.x must be at offset 12");
static_assert(offsetof(DisplayListItem, payload.begin.y) == 16,
              "begin.y must be at offset 16");
static_assert(offsetof(DisplayListItem, payload.begin.w) == 20,
              "begin.w must be at offset 20");
static_assert(offsetof(DisplayListItem, payload.begin.h) == 24,
              "begin.h must be at offset 24");

// Fill payload offsets
static_assert(offsetof(DisplayListItem, payload.fill.color) == 4,
              "fill.color must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.fill.clip_index) == 8,
              "fill.clip_index must be at offset 8");

// DrawView payload offsets
static_assert(offsetof(DisplayListItem, payload.draw_view.view_id) == 4,
              "draw_view.view_id must be at offset 4");

// Text payload offsets
static_assert(offsetof(DisplayListItem, payload.text.text_id) == 4,
              "text.text_id must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.text.box_index) == 8,
              "text.box_index must be at offset 8");

// Image payload offsets
static_assert(offsetof(DisplayListItem, payload.image.image_id) == 4,
              "image.image_id must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.image.box_index) == 8,
              "image.box_index must be at offset 8");

// Border payload offsets
static_assert(offsetof(DisplayListItem, payload.border.out_index) == 4,
              "border.out_index must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.border.inner_index) == 8,
              "border.inner_index must be at offset 8");
static_assert(offsetof(DisplayListItem, payload.border.colors) == 12,
              "border.colors must be at offset 12");
static_assert(offsetof(DisplayListItem, payload.border.styles) == 28,
              "border.styles must be at offset 28");

// RecordBox payload offsets
static_assert(offsetof(DisplayListItem, payload.record_box.x) == 4,
              "record_box.x must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.record_box.y) == 8,
              "record_box.y must be at offset 8");
static_assert(offsetof(DisplayListItem, payload.record_box.w) == 12,
              "record_box.w must be at offset 12");
static_assert(offsetof(DisplayListItem, payload.record_box.h) == 16,
              "record_box.h must be at offset 16");
static_assert(offsetof(DisplayListItem, payload.record_box.radii) == 20,
              "record_box.radii must be at offset 20");
static_assert(offsetof(DisplayListItem, payload.record_box.has_radii) == 52,
              "record_box.has_radii must be at offset 52");

// ClipRect payload offsets
static_assert(offsetof(DisplayListItem, payload.clip_rect.x) == 4,
              "clip_rect.x must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.clip_rect.y) == 8,
              "clip_rect.y must be at offset 8");
static_assert(offsetof(DisplayListItem, payload.clip_rect.w) == 12,
              "clip_rect.w must be at offset 12");
static_assert(offsetof(DisplayListItem, payload.clip_rect.h) == 16,
              "clip_rect.h must be at offset 16");
static_assert(offsetof(DisplayListItem, payload.clip_rect.radii) == 20,
              "clip_rect.radii must be at offset 20");
static_assert(offsetof(DisplayListItem, payload.clip_rect.has_radii) == 52,
              "clip_rect.has_radii must be at offset 52");

// LinearGradient payload offsets
static_assert(offsetof(DisplayListItem,
                       payload.linear_gradient.color_count_offset) == 4,
              "linear_gradient.color_count_offset must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.color_count) ==
                  8,
              "linear_gradient.color_count must be at offset 8");
static_assert(offsetof(DisplayListItem,
                       payload.linear_gradient.stop_count_offset) == 12,
              "linear_gradient.stop_count_offset must be at offset 12");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.stop_count) ==
                  16,
              "linear_gradient.stop_count must be at offset 16");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.tiling_index) ==
                  20,
              "linear_gradient.tiling_index must be at offset 20");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.clip_index) ==
                  24,
              "linear_gradient.clip_index must be at offset 24");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.repeat_x) == 28,
              "linear_gradient.repeat_x must be at offset 28");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.repeat_y) == 32,
              "linear_gradient.repeat_y must be at offset 32");
static_assert(offsetof(DisplayListItem, payload.linear_gradient.angle) == 36,
              "linear_gradient.angle must be at offset 36");

// BoxShadow payload offsets
static_assert(offsetof(DisplayListItem, payload.box_shadow.shadow_box_index) ==
                  4,
              "box_shadow.shadow_box_index must be at offset 4");
static_assert(offsetof(DisplayListItem, payload.box_shadow.clip_box_index) == 8,
              "box_shadow.clip_box_index must be at offset 8");
static_assert(offsetof(DisplayListItem, payload.box_shadow.color) == 12,
              "box_shadow.color must be at offset 12");
static_assert(offsetof(DisplayListItem, payload.box_shadow.blur_radius) == 16,
              "box_shadow.blur_radius must be at offset 16");
static_assert(offsetof(DisplayListItem, payload.box_shadow.clip_mode) == 20,
              "box_shadow.clip_mode must be at offset 20");

static_assert(std::is_standard_layout<DisplayListItem>::value,
              "DisplayListItem must be standard layout for JNI");
static_assert(
    std::is_trivially_copyable<DisplayListItem>::value,
    "DisplayListItem must be trivially copyable for DirectByteBuffer");
}  // extern "C"

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

  // New zero-copy buffer access
  const uint8_t* GetContentItemsData() const {
    return content_items_.has_value()
               ? reinterpret_cast<const uint8_t*>(content_items_->data())
               : nullptr;
  }
  size_t GetContentItemsSize() const {
    return content_items_.has_value() ? content_items_->size() : 0;
  }
  const uint8_t* GetContentData() const {
    return content_data_.has_value() ? content_data_->data() : nullptr;
  }
  size_t GetContentDataSize() const {
    return content_data_.has_value() ? content_data_->size() : 0;
  }

  const int32_t* GetContentOpTypesData() const {
    return legacy_content_data_.has_value() ? legacy_content_data_->ops.data()
                                            : nullptr;
  }
  const int32_t* GetContentIntData() const {
    return legacy_content_data_.has_value()
               ? legacy_content_data_->int_data.data()
               : nullptr;
  }
  const float* GetContentFloatData() const {
    return legacy_content_data_.has_value()
               ? legacy_content_data_->float_data.data()
               : nullptr;
  }
  size_t GetContentOpTypesSize() const {
    return legacy_content_data_.has_value() ? legacy_content_data_->ops.size()
                                            : 0;
  }
  bool HasContent() const { return GetContentOpTypesSize() != 0; }
  size_t GetContentIntDataSize() const {
    return legacy_content_data_.has_value()
               ? legacy_content_data_->int_data.size()
               : 0;
  }
  size_t GetContentFloatDataSize() const {
    return legacy_content_data_.has_value()
               ? legacy_content_data_->float_data.size()
               : 0;
  }
  int32_t GetOpAtIndex(size_t index) const {
    return legacy_content_data_->ops[index];
  }
  int32_t GetIntAtIndex(size_t index) const {
    return legacy_content_data_->int_data[index];
  }
  float GetFloatAtIndex(size_t index) const {
    return legacy_content_data_->float_data[index];
  }

  const float* GetRenderOffset() const { return render_offset_; }

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
    AddOperationToData(legacy_content_data_, type, args...);
  }

  void AppendItem(const DisplayListItem& item);

  void AddSubLayer(int id) { sub_layers_.emplace_back(id); }
  const auto& SubLayers() const { return sub_layers_; }

  void AddSubtreeProperty(const SubtreeProperty& prop) {
    subtree_properties_->push_back(prop);
  }

 private:
  template <typename OpType, typename... Args>
  void AddOperationToData(base::auto_create_optional<OpData>& data_store,
                          OpType type, Args... args);

  // Content operations (stable during animations) - lazy allocated
  base::auto_create_optional<base::InlineVector<DisplayListItem, 8>>
      content_items_;

  // Trailing variable-length data region for gradient colors/stops
  base::auto_create_optional<base::Vector<uint8_t>> content_data_;

  base::auto_create_optional<OpData> legacy_content_data_;

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

  OpData* op_data = &(*data_store);

  op_data->ops.push_back(static_cast<int32_t>(type));

  if constexpr (sizeof...(Args) == 0) {
    op_data->int_data.push_back(0);
    op_data->int_data.push_back(0);
  } else {
    constexpr size_t int_count =
        (... + (std::is_same_v<std::decay_t<Args>, int32_t> ? 1 : 0));
    constexpr size_t float_count =
        (... + (std::is_same_v<std::decay_t<Args>, float> ? 1 : 0));

    op_data->int_data.reserve(op_data->int_data.size() + 2 + int_count);
    if constexpr (float_count > 0) {
      op_data->float_data.reserve(op_data->float_data.size() + float_count);
    }

    op_data->int_data.push_back(static_cast<int32_t>(int_count));
    op_data->int_data.push_back(static_cast<int32_t>(float_count));

    ((std::is_same_v<std::decay_t<Args>, int32_t>
          ? op_data->int_data.push_back(static_cast<int32_t>(args))
          : op_data->float_data.push_back(static_cast<float>(args))),
     ...);
  }
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_H_
