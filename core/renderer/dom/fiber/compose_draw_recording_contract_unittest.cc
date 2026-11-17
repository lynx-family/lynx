// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// SIZE_OK: Frozen wire contracts share the actual direct compiler fixture.
#include <cstring>
#include <iostream>

#include "base/include/vector.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/testing/compose_draw_recording_contract_test_helpers.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/starlight/style/css_type.h"
#include "gfx/public/display_list_reader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx::tasm::testing {
namespace {
using contract_fixture::Case;
using contract_fixture::Change;
using contract_fixture::Fixtures;
using contract_fixture::Json;
using contract_fixture::Prefix;
struct PrefixSnapshot {
  base::ByteArray items, data;
  size_t boxes;
  bool clip;
  explicit PrefixSnapshot(const DisplayListBuilder& b)
      : boxes(b.BoxModelCount()),
        clip(b.CurrentDisplayList().RootNeedClipBounds()) {
    const auto& l = b.CurrentDisplayList();
    items.fill(l.GetContentItemsData(), l.GetContentItemsByteSize());
    data.fill(l.GetContentData(), l.GetContentDataSize());
  }
  void Expect(const DisplayListBuilder& b, bool extended = false) const {
    const auto& l = b.CurrentDisplayList();
    ASSERT_GE(l.GetContentItemsByteSize(), items.size());
    ASSERT_GE(l.GetContentDataSize(), data.size());
    EXPECT_EQ(std::memcmp(items.data(), l.GetContentItemsData(), items.size()),
              0);
    EXPECT_EQ(std::memcmp(data.data(), l.GetContentData(), data.size()), 0);
    EXPECT_EQ(l.RootNeedClipBounds(), clip);
    if (!extended) {
      EXPECT_EQ(l.GetContentItemsByteSize(), items.size());
      EXPECT_EQ(l.GetContentDataSize(), data.size());
      EXPECT_EQ(b.BoxModelCount(), boxes);
    }
  }
};
// Both box payloads have x/y/w/h followed by eight radii in the published ABI.
template <typename Box>
void ExpectBox(const Box& box, const rapidjson::Value& expected,
               double pixels) {
  const float actual[] = {box.x, box.y, box.w, box.h};
  ASSERT_EQ(expected.Size(), 12u);
  std::cout << " box=[";
  for (unsigned i = 0; i < 12; ++i) {
    const float v = i < 4 ? actual[i] : box.radii[i - 4];
    EXPECT_FLOAT_EQ(v, expected[i].GetDouble() / pixels);
    std::cout << (i ? "," : "") << v;
  }
  bool has_radii = false;
  for (unsigned i = 4; i < 12; ++i) has_radii |= expected[i].GetDouble() != 0;
  EXPECT_EQ(box.has_radii != 0, has_radii);
  std::cout << "]";
}
class ComposeDrawRecordingContractTest : public ::testing::Test {};
TEST_F(ComposeDrawRecordingContractTest,
       SharedPhysicalFixturesReachActualDisplayListAtDeclaredScales) {
  const auto v1 = Fixtures("recording-v1");
  for (const auto& expected : v1["polyfillAcceptance"].GetArray()) {
    const char* family = expected["family"].GetString();
    const auto fixtures = Fixtures(family);
    const auto& sample = Case(fixtures, expected["case"].GetString());
    const bool clip = sample.HasMember("mode") &&
                      std::strcmp(sample["mode"].GetString(), "op18") == 0;
    for (const auto& scale : expected["scales"].GetArray()) {
      SCOPED_TRACE(expected["id"].GetString());
      SCOPED_TRACE(scale.GetDouble());
      DisplayListBuilder builder;
      Prefix(&builder);
      const PrefixSnapshot before(builder);

      DisplayList compiled;
      bool shadow = false;
      const auto phase =
          clip ? ComposeDrawPhase::kClip : ComposeDrawPhase::kBehind;
      ASSERT_FALSE(detail::DecodeDrawDisplayList(
          Json(sample["wire"]), scale.GetDouble(), phase, &compiled, &shadow));

      ASSERT_TRUE(builder.AppendDrawingContent(compiled));
      before.Expect(builder, true);
      auto list = builder.Build();
      DisplayListReader reader(list);
      unsigned prefix = 0, boxes = 0, draws = 0;
      std::cout << "TASK13_DISPLAY id=" << expected["id"].GetString()
                << " pixels=" << scale.GetDouble();
      while (reader.HasNext()) {
        const auto& item = reader.Next();
        if (prefix++ < 3 || item.type == DisplayListOpType::kEnd) continue;
        if (item.type == DisplayListOpType::kRecordBox) {
          ASSERT_LT(boxes, expected["boxes"].Size());
          ExpectBox(item.payload.record_box, expected["boxes"][boxes++],
                    scale.GetDouble());
          continue;
        }
        ASSERT_LT(draws, expected["draws"].Size());
        const auto& draw = expected["draws"][draws++];
        const std::string kind = draw[0].GetString();
        std::cout << " draw=" << kind;
        if (kind == "fill") {
          ASSERT_EQ(item.type, DisplayListOpType::kFill);
          EXPECT_EQ(item.payload.fill.color, draw[1].GetUint());
          EXPECT_EQ(item.payload.fill.clip_index, draw[2].GetInt());
          std::cout << ":" << item.payload.fill.color << ":"
                    << item.payload.fill.clip_index;
        } else if (kind == "border") {
          ASSERT_EQ(item.type, DisplayListOpType::kBorder);
          const auto& b = item.payload.border;
          EXPECT_EQ(b.out_index, draw[2].GetInt());
          EXPECT_EQ(b.inner_index, draw[3].GetInt());
          for (unsigned i = 0; i < 4; ++i) {
            EXPECT_EQ(b.colors[i], draw[1].GetUint());
            EXPECT_EQ(b.styles[i],
                      static_cast<int32_t>(starlight::BorderStyleType::kSolid));
          }
        } else if (kind == "linear") {
          ASSERT_EQ(item.type, DisplayListOpType::kLinearGradient);
          const auto& g = item.payload.linear_gradient;
          EXPECT_FLOAT_EQ(g.angle, draw[1].GetDouble());
          EXPECT_EQ(g.tiling_index, draw[2].GetInt());
          EXPECT_EQ(g.clip_index, draw[3].GetInt());
          EXPECT_EQ(g.inner_clip_index_plus_one, draw[4].GetInt());
          EXPECT_EQ(g.repeat_x, 1);
          EXPECT_EQ(g.repeat_y, 1);
          ASSERT_EQ(g.color_count, draw[5].Size());
          ASSERT_EQ(g.stop_count, draw[6].Size());
          std::cout << ":" << g.angle << ":" << g.tiling_index << ":"
                    << g.clip_index << ":" << g.inner_clip_index_plus_one;
          for (unsigned i = 0; i < g.color_count; ++i) {
            EXPECT_EQ(reader.Colors(item)[i], draw[5][i].GetUint());
            EXPECT_FLOAT_EQ(reader.Stops(item)[i], draw[6][i].GetDouble());
            std::cout << " stop=" << reader.Colors(item)[i] << ":"
                      << reader.Stops(item)[i];
          }
        } else if (kind == "shadow") {
          ASSERT_EQ(item.type, DisplayListOpType::kBoxShadow);
          const auto& s = item.payload.box_shadow;
          EXPECT_EQ(s.color, draw[1].GetUint());
          EXPECT_FLOAT_EQ(s.blur_radius,
                          draw[2].GetDouble() / scale.GetDouble());
          EXPECT_EQ(s.shadow_box_index, draw[3].GetInt());
          EXPECT_EQ(s.clip_box_index, draw[4].GetInt());
          EXPECT_EQ(s.clip_mode, draw[5].GetInt());
          std::cout << ":" << s.color << ":" << s.blur_radius << ":"
                    << s.shadow_box_index << ":" << s.clip_box_index << ":"
                    << s.clip_mode;
        } else {
          ASSERT_EQ(kind, "clip");
          ASSERT_EQ(item.type, DisplayListOpType::kClipRect);
          ExpectBox(item.payload.clip_rect, draw[1], scale.GetDouble());
        }
      }
      std::cout << " boxes=" << boxes << " draws=" << draws << std::endl;
      EXPECT_EQ(boxes, expected["boxes"].Size());
      EXPECT_EQ(draws, expected["draws"].Size());
      EXPECT_EQ(builder.BoxModelCount(), 1u + boxes);
      EXPECT_EQ(shadow, !expected["draws"].Empty() &&
                            std::strcmp(expected["draws"][0][0].GetString(),
                                        "shadow") == 0);
    }
  }
}
TEST_F(ComposeDrawRecordingContractTest,
       AdvisoryFeatureDeclarationsEmitCanonicalDisplayListBytes) {
  const auto fixtures = Fixtures("recording-v1");
  const auto& canonical = Case(fixtures, "rect");
  size_t variants = 0;
  for (const auto& variant : fixtures["variants"].GetArray()) {
    const auto& changes = variant["changes"];
    if (std::strcmp(variant["base"].GetString(), "rect") != 0 ||
        changes.Size() != 1 || changes[0][0].Size() != 1 ||
        changes[0][0][0].GetUint() != 7)
      continue;
    ++variants;
    SCOPED_TRACE(variant["id"].GetString());
    for (double pixels : {1., 2., 3.}) {
      SCOPED_TRACE(pixels);
      DisplayListBuilder canonical_builder;
      Prefix(&canonical_builder);

      DisplayList canonical_list;
      bool canonical_shadow = false;
      ASSERT_FALSE(detail::DecodeDrawDisplayList(
          Json(canonical["wire"]), pixels, ComposeDrawPhase::kBehind,
          &canonical_list, &canonical_shadow));
      ASSERT_TRUE(canonical_builder.AppendDrawingContent(canonical_list));
      const PrefixSnapshot expected(canonical_builder);

      auto wire = Json(canonical["wire"]);
      Change(wire, changes);
      DisplayListBuilder builder;
      Prefix(&builder);
      DisplayList compiled;
      bool shadow = false;
      ASSERT_FALSE(detail::DecodeDrawDisplayList(
          wire, pixels, ComposeDrawPhase::kBehind, &compiled, &shadow));
      ASSERT_TRUE(builder.AppendDrawingContent(compiled));
      EXPECT_EQ(shadow, canonical_shadow);
      expected.Expect(builder);
      std::cout << "ADVISORY_DISPLAY id=" << variant["id"].GetString()
                << " pixels=" << pixels << " identical_bytes=1" << std::endl;
    }
  }
  EXPECT_EQ(variants, 6u);
}
TEST_F(ComposeDrawRecordingContractTest,
       FullUnsupportedAndUnknownWiresPreservePreparedSentinel) {
  const auto fixtures = Fixtures("recording-v1");
  auto reject = [&](lepus::Value wire, const char* id) {
    for (double pixels : {1., 2., 3.}) {
      SCOPED_TRACE(id);
      SCOPED_TRACE(pixels);

      DisplayListBuilder sentinel_builder;
      Prefix(&sentinel_builder);
      const PrefixSnapshot sentinel(sentinel_builder);
      DisplayList compiled = sentinel_builder.Build();
      bool shadow = false;
      const auto error = detail::DecodeDrawDisplayList(
          wire, pixels, ComposeDrawPhase::kBehind, &compiled, &shadow);
      ASSERT_TRUE(error);

      EXPECT_EQ(compiled.GetContentItemsByteSize(), sentinel.items.size());
      EXPECT_EQ(compiled.GetContentDataSize(), sentinel.data.size());
      EXPECT_EQ(std::memcmp(compiled.GetContentItemsData(),
                            sentinel.items.data(), sentinel.items.size()),
                0);
      EXPECT_EQ(std::memcmp(compiled.GetContentData(), sentinel.data.data(),
                            sentinel.data.size()),
                0);
      EXPECT_FALSE(shadow);

      std::cout << "TASK13_REJECT id=" << id << " pixels=" << pixels
                << " code=" << static_cast<int>(error->code) << std::endl;
    }
  };
  for (const auto& c : fixtures["cases"].GetArray())
    if (std::strcmp(c["expected"]["currentPolyfill"].GetString(),
                    "unsupported") == 0)
      reject(Json(c["wire"]), c["id"].GetString());
  for (const auto& v : fixtures["variants"].GetArray()) {
    const auto& base = Case(fixtures, v["base"].GetString());
    const auto& e = v["expected"];
    if (!(e.HasMember("currentPolyfill")
              ? std::strcmp(e["currentPolyfill"].GetString(), "unsupported") ==
                    0
              : std::strcmp(base["expected"]["currentPolyfill"].GetString(),
                            "unsupported") == 0))
      continue;
    auto wire = Json(base["wire"]);
    Change(wire, v["changes"]);
    reject(wire, v["id"].GetString());
  }
  for (const auto& e : fixtures["errors"].GetArray()) {
    auto wire = Json(Case(fixtures, e["base"].GetString())["wire"]);
    Change(wire, e["changes"]);
    reject(wire, e["id"].GetString());
  }
}

class ComposeDrawRecordingExactRejectionTest
    : public ComposeDrawRecordingContractTest,
      public ::testing::WithParamInterface<contract_fixture::UnsupportedScene> {
};

TEST_P(ComposeDrawRecordingExactRejectionTest,
       AuthoredWireHasExactDiagnosticAndPreservesOwnedSentinels) {
  const auto& scene = GetParam();
  rapidjson::Document input;
  input.Parse(scene.wire);
  ASSERT_FALSE(input.HasParseError());
  for (double pixels : {1., 2., 3., 3.5}) {
    SCOPED_TRACE(pixels);
    DisplayListBuilder sentinel_builder;
    Prefix(&sentinel_builder);
    const PrefixSnapshot sentinel(sentinel_builder);
    DisplayList output = sentinel_builder.Build();
    bool shadow = false;
    const auto error = detail::DecodeDrawDisplayList(
        Json(input), pixels, ComposeDrawPhase::kBehind, &output, &shadow);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, DrawRecordingError::Code::kUnsupported);
    EXPECT_EQ(error->command_index, scene.command_index);
    EXPECT_EQ(error->name_space, scene.name_space);

    EXPECT_EQ(output.GetContentItemsByteSize(), sentinel.items.size());
    EXPECT_EQ(output.GetContentDataSize(), sentinel.data.size());
    EXPECT_EQ(std::memcmp(output.GetContentItemsData(), sentinel.items.data(),
                          sentinel.items.size()),
              0);
    EXPECT_EQ(std::memcmp(output.GetContentData(), sentinel.data.data(),
                          sentinel.data.size()),
              0);
    EXPECT_FALSE(shadow);
  }
}

INSTANTIATE_TEST_SUITE_P(
    AuthoredScenes, ComposeDrawRecordingExactRejectionTest,
    ::testing::ValuesIn(contract_fixture::kUnsupportedScenes),
    [](const ::testing::TestParamInfo<contract_fixture::UnsupportedScene>&
           info) { return info.param.name; });
}  // namespace
}  // namespace lynx::tasm::testing
