// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_TESTING_COMPOSE_DRAW_RECORDING_CONTRACT_TEST_HELPERS_H_
#define CORE_RENDERER_DOM_FIBER_TESTING_COMPOSE_DRAW_RECORDING_CONTRACT_TEST_HELPERS_H_
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"
namespace lynx::tasm::testing::contract_fixture {
// Input-only JSON conversion, not a decoder or a lowering implementation.
inline lepus::Value Json(const rapidjson::Value& value) {
  if (value.IsNumber()) return lepus::Value(value.GetDouble());
  if (value.IsBool()) return lepus::Value(value.GetBool());
  if (value.IsString()) return lepus::Value(value.GetString());
  if (value.IsObject()) {
    auto dict = lepus::Dictionary::Create();
    for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i)
      dict->SetValue(i->name.GetString(), Json(i->value));
    return lepus::Value(std::move(dict));
  }
  if (!value.IsArray()) return {};
  auto array = lepus::CArray::Create();
  for (const auto& item : value.GetArray()) array->push_back(Json(item));
  return lepus::Value(std::move(array));
}
inline void Prefix(DisplayListBuilder* b) {
  RoundedRectangle box;
  box.SetX(7);
  box.SetY(11);
  box.SetWidth(13);
  box.SetHeight(17);
  int32_t index;
  b->Begin(101, PlatformRendererType::kUnknown, 1, 2, 80, 60);
  b->RecordBoxModel(box, index);
  b->LinearGradient(180, {0xff010203, 0xff040506}, {0, 1}, index, index, 1, 1);
  b->MarkRootNeedClipBounds();
}
}  // namespace lynx::tasm::testing::contract_fixture
#endif  // CORE_RENDERER_DOM_FIBER_TESTING_COMPOSE_DRAW_RECORDING_CONTRACT_TEST_HELPERS_H_
