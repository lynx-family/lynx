// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/display_list.h"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

namespace lynx {
namespace tasm {

uint32_t DisplayList::RetainPath(fml::RefPtr<gfx::Path> path) {
  if (!path) return std::numeric_limits<uint32_t>::max();
  auto& paths = *paths_;
  for (size_t i = 0; i < paths.size(); ++i) {
    if (paths[i].get() == path.get()) return static_cast<uint32_t>(i);
  }
  if (paths.size() >= std::numeric_limits<uint32_t>::max()) std::abort();
  auto index = static_cast<uint32_t>(paths.size());
  // Skip an intermediate two-slot allocation when the inline slot fills.
  if (index == 1) paths.reserve(4);
  paths.push_back(std::move(path));
  return index;
}
const gfx::Path* DisplayList::GetPath(uint32_t index) const {
  return index < GetPathCount() ? (*paths_)[index].get() : nullptr;
}
size_t DisplayList::GetPathCount() const {
  return paths_.has_value() ? paths_->size() : 0;
}

void DisplayList::Reserve(int32_t capacity) {
  constexpr static const int32_t kPreAllocatedCapacityForItems = 10;
  constexpr static const int32_t kPreAllocatedCapacityForData = 64;

  content_items_->reserve(capacity * kPreAllocatedCapacityForItems);
  content_data_->reserve(capacity * kPreAllocatedCapacityForData);
}

void DisplayList::Clear() {
  paths_.reset();
  if (content_items_.has_value()) {
    content_items_->clear();
  }
  if (content_data_.has_value()) {
    content_data_->clear();
  }
  ClearSubtreeProperties();
}

void DisplayList::ClearSubtreeProperties() {
  if (subtree_properties_.has_value()) {
    subtree_properties_->clear();
    subtree_properties_.reset();
  }
}

void DisplayList::AppendItem(const DisplayListItem& item) {
  content_items_->push_back(item);
}

bool DisplayList::CanAppendDrawingContentSizes(
    size_t item_count, size_t data_size, size_t source_item_count,
    size_t source_data_size, size_t box_index_base, size_t source_box_count) {
  constexpr size_t kMax = std::numeric_limits<int32_t>::max();
  constexpr size_t kMaxItems = kMax / sizeof(DisplayListItem);
  return item_count <= kMaxItems &&
         source_item_count <= kMaxItems - item_count && data_size <= kMax &&
         source_data_size <= kMax - data_size && box_index_base <= kMax &&
         source_box_count <= kMax - box_index_base;
}

bool DisplayList::AppendDrawingContent(const DisplayList& source,
                                       int32_t box_index_base,
                                       size_t& appended_box_count) {
  const size_t item_count = GetContentItemsSize();
  const size_t data_size = GetContentDataSize();
  const size_t source_item_count = source.GetContentItemsSize();
  const size_t source_data_size = source.GetContentDataSize();
  if (this == &source || box_index_base < 0 || source.render_offset_[0] != 0 ||
      source.render_offset_[1] != 0 || source.GetSubtreePropertiesSize() != 0 ||
      !source.sub_layers_.empty() || source.GetPathCount() != 0 ||
      !source.images_.empty() || source.root_need_clip_bounds_ ||
      !CanAppendDrawingContentSizes(item_count, data_size, source_item_count,
                                    source_data_size, box_index_base, 0)) {
    return false;
  }

  size_t box_count = 0;
  const auto prior_box = [&box_count](int32_t index) {
    return index >= 0 && static_cast<size_t>(index) < box_count;
  };
  const auto valid_range = [source_data_size, data_size](uint32_t offset,
                                                         uint32_t count) {
    if (count == 0) return offset == 0;
    return offset % sizeof(uint32_t) == 0 &&
           data_size % sizeof(uint32_t) == 0 && offset <= source_data_size &&
           count <= (source_data_size - offset) / sizeof(uint32_t);
  };
  const auto* source_items =
      reinterpret_cast<const DisplayListItem*>(source.GetContentItemsData());
  for (size_t i = 0; i < source_item_count; ++i) {
    const auto& item = source_items[i];
    switch (item.type) {
      case DisplayListOpType::kRecordBox:
        ++box_count;
        break;
      case DisplayListOpType::kFill:
        if (item.payload.fill.clip_index != -1 &&
            !prior_box(item.payload.fill.clip_index)) {
          return false;
        }
        break;
      case DisplayListOpType::kBorder:
        if (!prior_box(item.payload.border.out_index) ||
            !prior_box(item.payload.border.inner_index)) {
          return false;
        }
        break;
      case DisplayListOpType::kLinearGradient: {
        const auto& gradient = item.payload.linear_gradient;
        if (!prior_box(gradient.tiling_index) ||
            !prior_box(gradient.clip_index) ||
            gradient.inner_clip_index_plus_one < 0 ||
            static_cast<size_t>(gradient.inner_clip_index_plus_one) >
                box_count ||
            !valid_range(gradient.color_count_offset, gradient.color_count) ||
            !valid_range(gradient.stop_count_offset, gradient.stop_count)) {
          return false;
        }
        break;
      }
      case DisplayListOpType::kBoxShadow: {
        const auto& shadow = item.payload.box_shadow;
        if (!prior_box(shadow.shadow_box_index) ||
            !prior_box(shadow.clip_box_index) ||
            (shadow.clip_mode != 0 && shadow.clip_mode != 1)) {
          return false;
        }
        break;
      }
      case DisplayListOpType::kClipRect:
        break;
      default:
        return false;
    }
  }
  if (!CanAppendDrawingContentSizes(item_count, data_size, source_item_count,
                                    source_data_size, box_index_base,
                                    box_count)) {
    return false;
  }

  if (source_data_size != 0) {
    content_data_->append(source.GetContentData(), source_data_size);
  }
  if (source_item_count != 0) {
    content_items_->append(source_items,
                           source_item_count * sizeof(DisplayListItem));
    for (size_t i = item_count; i < content_items_->size(); ++i) {
      auto& item = (*content_items_)[i];
      switch (item.type) {
        case DisplayListOpType::kFill:
          if (item.payload.fill.clip_index != -1)
            item.payload.fill.clip_index += box_index_base;
          break;
        case DisplayListOpType::kBorder:
          item.payload.border.out_index += box_index_base;
          item.payload.border.inner_index += box_index_base;
          break;
        case DisplayListOpType::kLinearGradient: {
          auto& gradient = item.payload.linear_gradient;
          gradient.tiling_index += box_index_base;
          gradient.clip_index += box_index_base;
          if (gradient.inner_clip_index_plus_one != 0)
            gradient.inner_clip_index_plus_one += box_index_base;
          if (gradient.color_count != 0)
            gradient.color_count_offset += static_cast<uint32_t>(data_size);
          if (gradient.stop_count != 0)
            gradient.stop_count_offset += static_cast<uint32_t>(data_size);
          break;
        }
        case DisplayListOpType::kBoxShadow:
          item.payload.box_shadow.shadow_box_index += box_index_base;
          item.payload.box_shadow.clip_box_index += box_index_base;
          break;
        default:
          break;
      }
    }
  }
  appended_box_count = box_count;
  return true;
}

void DisplayList::AddLinearGradient(float angle,
                                    const base::Vector<uint32_t>& colors,
                                    const base::Vector<float>& stops,
                                    int32_t tiling_index, int32_t clip_index,
                                    int32_t repeat_x, int32_t repeat_y,
                                    int32_t inner_clip_index) {
  // Reject before appending either items or trailing data, without overflow.
  if (inner_clip_index < -1 ||
      inner_clip_index == std::numeric_limits<int32_t>::max()) {
    return;
  }
  DisplayListItem item{};
  item.type = DisplayListOpType::kLinearGradient;

  uint32_t color_count = static_cast<uint32_t>(colors.size());
  uint32_t stop_count = static_cast<uint32_t>(stops.size());

  // Append colors and stops to the trailing data region
  uint32_t color_offset = 0;
  uint32_t stop_offset = 0;

  if (color_count > 0) {
    color_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(colors.data()),
                          colors.size() * sizeof(uint32_t));
  }

  if (stop_count > 0) {
    stop_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(stops.data()),
                          stops.size() * sizeof(float));
  }

  item.payload.linear_gradient.color_count_offset = color_offset;
  item.payload.linear_gradient.color_count = color_count;
  item.payload.linear_gradient.stop_count_offset = stop_offset;
  item.payload.linear_gradient.stop_count = stop_count;
  item.payload.linear_gradient.tiling_index = tiling_index;
  item.payload.linear_gradient.clip_index = clip_index;
  item.payload.linear_gradient.repeat_x = repeat_x;
  item.payload.linear_gradient.repeat_y = repeat_y;
  item.payload.linear_gradient.angle = angle;
  item.payload.linear_gradient.inner_clip_index_plus_one = inner_clip_index + 1;

  AppendItem(item);
}

void DisplayList::AddRadialGradient(float center_x, float center_y,
                                    float radius_x, float radius_y,
                                    const base::Vector<uint32_t>& colors,
                                    const base::Vector<float>& stops,
                                    int32_t tiling_index, int32_t clip_index,
                                    int32_t repeat_x, int32_t repeat_y) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kRadialGradient;

  const uint32_t color_count = static_cast<uint32_t>(colors.size());
  const uint32_t stop_count = static_cast<uint32_t>(stops.size());
  uint32_t color_offset = 0;
  uint32_t stop_offset = 0;
  if (color_count > 0) {
    color_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(colors.data()),
                          colors.size() * sizeof(uint32_t));
  }
  if (stop_count > 0) {
    stop_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(stops.data()),
                          stops.size() * sizeof(float));
  }

  auto& payload = item.payload.radial_gradient;
  payload.color_count_offset = color_offset;
  payload.color_count = color_count;
  payload.stop_count_offset = stop_offset;
  payload.stop_count = stop_count;
  payload.tiling_index = tiling_index;
  payload.clip_index = clip_index;
  payload.repeat_x = repeat_x;
  payload.repeat_y = repeat_y;
  payload.center_x = center_x;
  payload.center_y = center_y;
  payload.radius_x = radius_x;
  payload.radius_y = radius_y;
  AppendItem(item);
}

void DisplayList::AddBackgroundImage(fml::RefPtr<PaintImage> image,
                                     int32_t tiling_index, int32_t clip_index,
                                     int32_t repeat_x, int32_t repeat_y,
                                     BackgroundSizeMode size_mode,
                                     float position_x, float position_y) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kBackgroundImage;
  item.payload.background_image.image_id = image->image_key_;
  item.payload.background_image.tiling_index = tiling_index;
  item.payload.background_image.clip_index = clip_index;
  item.payload.background_image.repeat_x = repeat_x;
  item.payload.background_image.repeat_y = repeat_y;
  item.payload.background_image.size_mode = size_mode;
  item.payload.background_image.position_x = position_x;
  item.payload.background_image.position_y = position_y;
  AppendItem(item);
  Images().emplace_back(std::move(image));
}

}  // namespace tasm
}  // namespace lynx
