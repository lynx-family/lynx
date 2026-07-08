// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/android/display_list_item_serializer.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace lynx::tasm {
namespace {

enum class ItemShape {
  kMixed,
  kLight,
  kHeavy,
};

DisplayListItem MakeBeginItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kBegin;
  item.payload.begin.id = static_cast<int32_t>(index);
  item.payload.begin.type = static_cast<int32_t>(index % 7);
  item.payload.begin.x = static_cast<float>(index);
  item.payload.begin.y = static_cast<float>(index + 1);
  item.payload.begin.w = static_cast<float>(index + 2);
  item.payload.begin.h = static_cast<float>(index + 3);
  return item;
}

DisplayListItem MakeFillItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = 0xff000000u | static_cast<uint32_t>(index);
  item.payload.fill.clip_index = static_cast<int32_t>(index % 11);
  return item;
}

DisplayListItem MakeDrawViewItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kDrawView;
  item.payload.draw_view.view_id = static_cast<int32_t>(index);
  return item;
}

DisplayListItem MakeTextItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kText;
  item.payload.text.text_id = static_cast<int32_t>(index);
  item.payload.text.box_index = static_cast<int32_t>(index + 1);
  return item;
}

DisplayListItem MakeImageItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kImage;
  item.payload.image.image_id = static_cast<int32_t>(index);
  item.payload.image.box_index = static_cast<int32_t>(index + 1);
  return item;
}

DisplayListItem MakeBorderItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kBorder;
  item.payload.border.out_index = static_cast<int32_t>(index);
  item.payload.border.inner_index = static_cast<int32_t>(index + 1);
  for (int i = 0; i < 4; ++i) {
    item.payload.border.colors[i] =
        0xff000000u | static_cast<uint32_t>(index + i);
    item.payload.border.styles[i] = i;
  }
  return item;
}

DisplayListItem MakeClipRectItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kClipRect;
  item.payload.clip_rect.x = static_cast<float>(index);
  item.payload.clip_rect.y = static_cast<float>(index + 1);
  item.payload.clip_rect.w = static_cast<float>(index + 2);
  item.payload.clip_rect.h = static_cast<float>(index + 3);
  for (int i = 0; i < 8; ++i) {
    item.payload.clip_rect.radii[i] = static_cast<float>(index + i);
  }
  item.payload.clip_rect.has_radii = 1;
  return item;
}

DisplayListItem MakeRecordBoxItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kRecordBox;
  item.payload.record_box.x = static_cast<float>(index);
  item.payload.record_box.y = static_cast<float>(index + 1);
  item.payload.record_box.w = static_cast<float>(index + 2);
  item.payload.record_box.h = static_cast<float>(index + 3);
  for (int i = 0; i < 8; ++i) {
    item.payload.record_box.radii[i] = static_cast<float>(index + i);
  }
  item.payload.record_box.has_radii = 1;
  return item;
}

DisplayListItem MakeLinearGradientItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kLinearGradient;
  item.payload.linear_gradient.color_count_offset =
      static_cast<uint32_t>(index);
  item.payload.linear_gradient.color_count = 4;
  item.payload.linear_gradient.stop_count_offset =
      static_cast<uint32_t>(index + 4);
  item.payload.linear_gradient.stop_count = 4;
  item.payload.linear_gradient.tiling_index = static_cast<int32_t>(index % 3);
  item.payload.linear_gradient.clip_index = static_cast<int32_t>(index % 5);
  item.payload.linear_gradient.repeat_x = static_cast<int32_t>(index % 2);
  item.payload.linear_gradient.repeat_y = static_cast<int32_t>((index + 1) % 2);
  item.payload.linear_gradient.angle = static_cast<float>(index);
  return item;
}

DisplayListItem MakeBoxShadowItem(size_t index) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kBoxShadow;
  item.payload.box_shadow.shadow_box_index = static_cast<int32_t>(index);
  item.payload.box_shadow.clip_box_index = static_cast<int32_t>(index + 1);
  item.payload.box_shadow.color = 0xff000000u | static_cast<uint32_t>(index);
  item.payload.box_shadow.blur_radius = static_cast<float>(index % 16);
  item.payload.box_shadow.clip_mode = static_cast<int32_t>(index % 3);
  return item;
}

DisplayListItem MakeMixedItem(size_t index) {
  switch (index % 10) {
    case 0:
      return MakeBeginItem(index);
    case 1:
      return MakeFillItem(index);
    case 2:
      return MakeDrawViewItem(index);
    case 3:
      return MakeTextItem(index);
    case 4:
      return MakeImageItem(index);
    case 5:
      return MakeBorderItem(index);
    case 6:
      return MakeClipRectItem(index);
    case 7:
      return MakeRecordBoxItem(index);
    case 8:
      return MakeLinearGradientItem(index);
    default:
      return MakeBoxShadowItem(index);
  }
}

std::vector<DisplayListItem> BuildItems(size_t count, ItemShape shape) {
  std::vector<DisplayListItem> items;
  items.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    switch (shape) {
      case ItemShape::kMixed:
        items.push_back(MakeMixedItem(i));
        break;
      case ItemShape::kLight:
        items.push_back(i % 2 == 0 ? MakeDrawViewItem(i) : MakeFillItem(i));
        break;
      case ItemShape::kHeavy:
        items.push_back(i % 2 == 0 ? MakeClipRectItem(i)
                                   : MakeRecordBoxItem(i));
        break;
    }
  }
  return items;
}

void SetCounters(benchmark::State& state, size_t item_count) {
  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(item_count));
  state.SetBytesProcessed(
      static_cast<int64_t>(state.iterations()) *
      static_cast<int64_t>(kAndroidDisplayListItemBufferHeaderSize +
                           item_count * kSerializedAndroidDisplayListItemSize));
}

void RunSerializeBenchmark(benchmark::State& state, ItemShape shape) {
  const size_t item_count = static_cast<size_t>(state.range(0));
  const auto items = BuildItems(item_count, shape);
  base::Vector<uint8_t> buffer;
  SerializeDisplayListItemsForAndroid(items.data(), items.size(), buffer);

  for (auto _ : state) {
    SerializeDisplayListItemsForAndroid(items.data(), items.size(), buffer);
    benchmark::DoNotOptimize(buffer.data());
    benchmark::ClobberMemory();
  }
  SetCounters(state, item_count);
}

void RunColdSerializeBenchmark(benchmark::State& state, ItemShape shape) {
  const size_t item_count = static_cast<size_t>(state.range(0));
  const auto items = BuildItems(item_count, shape);

  for (auto _ : state) {
    base::Vector<uint8_t> buffer;
    SerializeDisplayListItemsForAndroid(items.data(), items.size(), buffer);
    benchmark::DoNotOptimize(buffer.data());
    benchmark::ClobberMemory();
  }
  SetCounters(state, item_count);
}

void ApplyItemCounts(benchmark::internal::Benchmark* benchmark) {
  benchmark->Arg(1)->Arg(8)->Arg(64)->Arg(512)->Arg(2048);
}

void BM_AndroidDisplayListSerializeMixed(benchmark::State& state) {
  RunSerializeBenchmark(state, ItemShape::kMixed);
}

void BM_AndroidDisplayListSerializeLight(benchmark::State& state) {
  RunSerializeBenchmark(state, ItemShape::kLight);
}

void BM_AndroidDisplayListSerializeHeavy(benchmark::State& state) {
  RunSerializeBenchmark(state, ItemShape::kHeavy);
}

void BM_AndroidDisplayListSerializeMixedColdBuffer(benchmark::State& state) {
  RunColdSerializeBenchmark(state, ItemShape::kMixed);
}

BENCHMARK(BM_AndroidDisplayListSerializeMixed)->Apply(ApplyItemCounts);
BENCHMARK(BM_AndroidDisplayListSerializeLight)->Apply(ApplyItemCounts);
BENCHMARK(BM_AndroidDisplayListSerializeHeavy)->Apply(ApplyItemCounts);
BENCHMARK(BM_AndroidDisplayListSerializeMixedColdBuffer)
    ->Apply(ApplyItemCounts);

}  // namespace
}  // namespace lynx::tasm
