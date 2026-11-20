// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_TESTING_COMPOSE_DRAW_RECORDING_CONTRACT_TEST_HELPERS_H_
#define CORE_RENDERER_DOM_FIBER_TESTING_COMPOSE_DRAW_RECORDING_CONTRACT_TEST_HELPERS_H_
#include <cstring>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"
namespace lynx::tasm::testing::contract_fixture {
#include "core/renderer/dom/fiber/testing/compose_draw_recording_fixtures.inc"
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
inline rapidjson::Document Fixtures(const char* family) {
  rapidjson::Document d;
  for (const auto& f : kComposeDrawRecordingFixtures)
    if (std::strcmp(f.family, family) == 0) d.Parse(f.json);
  EXPECT_FALSE(d.HasParseError());
  return d;
}
inline const rapidjson::Value& Case(const rapidjson::Value& fixtures,
                                    const char* id) {
  for (const auto& row : fixtures["cases"].GetArray())
    if (std::strcmp(row["id"].GetString(), id) == 0) return row;
  ADD_FAILURE() << "Missing canonical input " << id;
  return fixtures["cases"][0];
}
inline void Change(lepus::Value wire, const rapidjson::Value& changes) {
  for (const auto& change : changes.GetArray()) {
    auto target = wire;
    const auto& indices = change[0];
    for (rapidjson::SizeType i = 0; i + 1 < indices.Size(); ++i)
      target = target.GetProperty(indices[i].GetUint());
    ASSERT_TRUE(target.Array()->set(indices[indices.Size() - 1].GetUint(),
                                    Json(change[1])));
  }
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
// Frozen authored PP inputs at physical 224x140, including the visible red
// prefix. Diagnostic expectations name the checked boundary independently of
// the decoder. Sweep has no filter, stroke, blend or transform confounder.
struct UnsupportedScene {
  const char* name;
  const char* wire;
  size_t command_index;
  const char* name_space;
  bool decode_rejects;
};
inline constexpr UnsupportedScene kUnsupportedScenes[] = {
    {"Radial",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[1,1,1]],[[1,0,0,224,140],[1,0,0,224,140]],[[[1,4294901760],[1,0,true,null],[1],[]],[[3,112,70,70,[4294901760,4278190335],[0,1],0,[1,0,0,1,0,0]],[0,0,true,null],[1],[]]],[],["paint.blend.SrcOver","paint.radial","paint.solid","paint.srgb","paint.tile.Clamp","path.rect"],[]])json",
     DrawRecordingError::kNoCommand, "paint.radial", false},
    {"Sweep",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[1,1,1]],[[1,0,0,224,140],[1,0,0,224,140]],[[[1,4294901760],[1,0,true,null],[1],[]],[[4,112,70,[4294901760,4278190335],[0,1],[1,0,0,1,0,0]],[0,0,true,null],[1],[]]],[],["paint.blend.SrcOver","paint.solid","paint.srgb","paint.sweep","path.rect"],[]])json",
     DrawRecordingError::kNoCommand, "paint.sweep", false},
    {"Stroke",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[1,1,1]],[[1,0,0,224,140],[1,0,0,224,140]],[[[1,4294901760],[1,0,true,null],[1],[]],[[1,4278190335],[0,0,true,null],[2,0,0,0,4,[],0,null],[]]],[],["paint.blend.SrcOver","paint.solid","paint.srgb","paint.stroke","path.rect"],[]])json",
     DrawRecordingError::kNoCommand, "paint.stroke", false},
    {"Ellipse",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[1,1,1]],[[1,0,0,224,140],[2,0,0,224,140,12,4,12,4,12,4,12,4]],[[[1,4294901760],[1,0,true,null],[1],[]],[[1,4278190335],[0,0,true,null],[1],[]]],[],["paint.blend.SrcOver","paint.solid","paint.srgb","path.rect","path.roundrect"],[]])json",
     DrawRecordingError::kNoCommand, "", false},
    {"Transform",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[2],[4,1,0,0,1,1,2],[1,1,1],[3]],[[1,0,0,224,140],[1,0,0,224,140]],[[[1,4294901760],[1,0,true,null],[1],[]],[[1,4278190335],[0,0,true,null],[1],[]]],[],["drawing.state","paint.blend.SrcOver","paint.solid","paint.srgb","path.rect"],[]])json",
     2, "", false},
    {"CallbackClip",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[2],[5,1,0,true],[1,2,1],[3]],[[1,0,0,224,140],[1,0,0,0,140],[1,0,0,224,140]],[[[1,4294901760],[1,0,true,null],[1],[]],[[1,4278190335],[0,0,true,null],[1],[]]],[],["drawing.clip","drawing.state","paint.blend.SrcOver","paint.solid","paint.srgb","path.rect"],[]])json",
     2, "drawing.clip", false},
    {"Tail",
     R"json(["lynx.draw.recording",1,0,[[1,0,0],[255,"scene.unsupported-tail",1,"retain-me",[]]],[[1,0,0,224,140]],[[[1,4294901760],[1,0,true,null],[1],[]]],[],["extension.scene.unsupported-tail.v1","paint.blend.SrcOver","paint.solid","paint.srgb","path.rect"],[]])json",
     DrawRecordingError::kNoCommand, "scene.unsupported-tail", true},
};
}  // namespace lynx::tasm::testing::contract_fixture
#endif  // CORE_RENDERER_DOM_FIBER_TESTING_COMPOSE_DRAW_RECORDING_CONTRACT_TEST_HELPERS_H_
