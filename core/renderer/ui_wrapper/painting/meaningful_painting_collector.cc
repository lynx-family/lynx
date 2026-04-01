// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/meaningful_painting_collector.h"

#include <utility>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace lynx::tasm {
namespace {

struct BoxRecord {
  float x = 0.f;
  float y = 0.f;
  float width = 0.f;
  float height = 0.f;
};

struct TranslationState {
  float x = 0.f;
  float y = 0.f;
};

int32_t ToInt(float value) { return static_cast<int32_t>(value); }

fml::RefPtr<PlatformRendererImpl> FindChildRenderer(
    const base::InlineVector<fml::RefPtr<PlatformRenderer>,
                             kChildrenInlineVectorSize>& children,
    size_t* next_child_index, int32_t view_id) {
  if (next_child_index == nullptr) {
    return nullptr;
  }

  if (*next_child_index < children.size() &&
      children[*next_child_index] != nullptr &&
      children[*next_child_index]->GetId() == view_id) {
    return fml::static_ref_ptr_cast<PlatformRendererImpl>(
        children[(*next_child_index)++]);
  }

  for (size_t i = *next_child_index; i < children.size(); ++i) {
    if (children[i] != nullptr && children[i]->GetId() == view_id) {
      *next_child_index = i + 1;
      return fml::static_ref_ptr_cast<PlatformRendererImpl>(children[i]);
    }
  }

  if (*next_child_index < children.size() &&
      children[*next_child_index] != nullptr) {
    return fml::static_ref_ptr_cast<PlatformRendererImpl>(
        children[(*next_child_index)++]);
  }

  return nullptr;
}

}  // namespace

MeaningfulPaintingCollector::MeaningfulPaintingCollector(
    const fml::RefPtr<PlatformRendererImpl>& root_renderer)
    : root_renderer_(root_renderer) {}

base::Vector<MeaningfulPaintingAreaRecord>
MeaningfulPaintingCollector::Collect() const {
  base::Vector<MeaningfulPaintingAreaRecord> records;
  if (root_renderer_ == nullptr) {
    return records;
  }

  CollectFromRenderer(root_renderer_, 0.f, 0.f, &records);
  return records;
}

base::Vector<int32_t> MeaningfulPaintingCollector::Serialize(
    const base::Vector<MeaningfulPaintingAreaRecord>& records) {
  base::Vector<int32_t> flattened;
  flattened.reserve(records.size() *
                    kMeaningfulPaintingAreaSerializedFieldCount);
  for (const auto& record : records) {
    flattened.push_back(record.sign);
    flattened.push_back(static_cast<int32_t>(record.type));
    flattened.push_back(record.x);
    flattened.push_back(record.y);
    flattened.push_back(record.width);
    flattened.push_back(record.height);
  }
  return flattened;
}

void MeaningfulPaintingCollector::CollectFromRenderer(
    const fml::RefPtr<PlatformRendererImpl>& renderer, float parent_offset_x,
    float parent_offset_y,
    base::Vector<MeaningfulPaintingAreaRecord>* out_records) const {
  if (renderer == nullptr || out_records == nullptr) {
    return;
  }

  const auto& display_list = renderer->GetDisplayList();
  auto* ops = display_list.GetContentOpTypesData();
  auto* int_data = display_list.GetContentIntData();
  auto* float_data = display_list.GetContentFloatData();
  if (ops == nullptr || int_data == nullptr || float_data == nullptr) {
    return;
  }

  const auto& children = renderer->Children();
  size_t child_renderer_idx = 0;
  size_t ops_idx = 0;
  size_t int_data_idx = 0;
  size_t float_data_idx = 0;

  // Text/Image ops only point to RecordBox indices, so we keep the resolved
  // box geometry from the current display list and reuse it when those ops are
  // visited later in the same renderer.
  std::vector<BoxRecord> box_records;
  // Nested OP_BEGIN blocks contribute additional local translation on top of
  // the renderer's fragment frame.
  std::vector<TranslationState> translation_stack;
  bool has_root_begin = false;
  float renderer_origin_x = 0.f;
  float renderer_origin_y = 0.f;
  float current_translation_x = 0.f;
  float current_translation_y = 0.f;

  while (ops_idx < display_list.GetContentOpTypesSize()) {
    const int32_t op = ops[ops_idx++];
    const int32_t int_param_cnt = int_data[int_data_idx++];
    const int32_t float_param_cnt = int_data[int_data_idx++];
    const size_t int_param_end = int_data_idx + int_param_cnt;
    const size_t float_param_end = float_data_idx + float_param_cnt;

    switch (static_cast<DisplayListOpType>(op)) {
      case DisplayListOpType::kBegin: {
        int_data_idx += (int_param_cnt > 0) ? 1 : 0;

        float x = 0.f;
        float y = 0.f;
        if (float_param_cnt >= 4) {
          x = float_data[float_data_idx++];
          y = float_data[float_data_idx++];
          float_data_idx += 2;  // width, height
        }

        if (!has_root_begin) {
          // The root OP_BEGIN frame is the actual fragment placement. Android
          // may offset the host view by render_offset during drawing, but
          // meaningful content geometry still follows the fragment frame.
          has_root_begin = true;
          renderer_origin_x = x;
          renderer_origin_y = y;
          translation_stack.push_back({0.f, 0.f});
          current_translation_x = 0.f;
          current_translation_y = 0.f;
        } else {
          current_translation_x += x;
          current_translation_y += y;
          translation_stack.push_back(
              {current_translation_x, current_translation_y});
        }
        break;
      }
      case DisplayListOpType::kEnd: {
        if (!translation_stack.empty()) {
          translation_stack.pop_back();
        }
        if (translation_stack.empty()) {
          current_translation_x = 0.f;
          current_translation_y = 0.f;
        } else {
          current_translation_x = translation_stack.back().x;
          current_translation_y = translation_stack.back().y;
        }
        break;
      }
      case DisplayListOpType::kRecordBox: {
        if (float_param_cnt >= 4) {
          const float left = float_data[float_data_idx++];
          const float top = float_data[float_data_idx++];
          const float width = float_data[float_data_idx++];
          const float height = float_data[float_data_idx++];
          BoxRecord box_record;
          // RecordBox stores local box geometry inside the current display
          // list. We resolve it into absolute page coordinates by combining the
          // ancestor renderer offset, the current renderer frame, and nested
          // Begin translations.
          box_record.x = parent_offset_x + renderer_origin_x +
                         current_translation_x + left;
          box_record.y =
              parent_offset_y + renderer_origin_y + current_translation_y + top;
          box_record.width = width;
          box_record.height = height;
          box_records.push_back(box_record);
        }
        break;
      }
      case DisplayListOpType::kText:
      case DisplayListOpType::kImage: {
        if (int_param_cnt >= 2) {
          const int32_t sign = int_data[int_data_idx++];
          const int32_t box_index = int_data[int_data_idx++];
          // Text/Image ops only reference a previously recorded box index, so
          // the meaningful area is emitted after resolving that shared box.
          if (box_index >= 0 &&
              static_cast<size_t>(box_index) < box_records.size()) {
            const auto& box = box_records[box_index];
            MeaningfulPaintingAreaRecord record;
            record.sign = sign;
            record.type = op == static_cast<int32_t>(DisplayListOpType::kText)
                              ? PlatformRendererType::kText
                              : PlatformRendererType::kImage;
            record.x = ToInt(box.x);
            record.y = ToInt(box.y);
            record.width = ToInt(box.width);
            record.height = ToInt(box.height);
            out_records->push_back(record);
          }
        }
        break;
      }
      case DisplayListOpType::kDrawView: {
        if (int_param_cnt >= 1) {
          const int32_t view_id = int_data[int_data_idx++];
          auto child_renderer =
              FindChildRenderer(children, &child_renderer_idx, view_id);
          // DrawView switches to the child renderer's display list while
          // preserving the accumulated offset from ancestor fragments.
          CollectFromRenderer(
              child_renderer,
              parent_offset_x + renderer_origin_x + current_translation_x,
              parent_offset_y + renderer_origin_y + current_translation_y,
              out_records);
        }
        break;
      }
      default:
        break;
    }

    int_data_idx = int_param_end;
    float_data_idx = float_param_end;
  }
}

}  // namespace lynx::tasm
