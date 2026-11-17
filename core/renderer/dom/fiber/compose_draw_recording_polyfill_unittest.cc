// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>
#include <cstring>

#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fiber/testing/compose_draw_recording_contract_test_helpers.h"
#include "core/renderer/starlight/style/css_type.h"
#include "gfx/public/display_list_reader.h"

namespace lynx::tasm::testing {
namespace {
using Code = DrawRecordingError::Code;
using Value = lepus::Value;
using contract_fixture::Prefix;
Value Parse(const std::string& text) {
  rapidjson::Document document;
  document.Parse(text.c_str());
  EXPECT_FALSE(document.HasParseError());
  return contract_fixture::Json(document);
}
Value Wire(
    const std::string& commands, const std::string& paths,
    const std::string& paint = "[[1,2148676694],[0.5,0,true,null],[1],[]]") {
  return Parse("[\"lynx.draw.recording\",1,0," + commands + "," + paths + ",[" +
               paint + "],[],[],[]]");
}
Value Solid(double alpha = .5, bool hole = false) {
  return Wire(
      hole ? "[[1,2,0]]" : "[[1,0,0]]",
      hole ? "[[1,0,0,40,20],[1,2,2,36,16],[3,0,1]]" : "[[1,0,0,40,20]]",
      "[[1,2148676694],[" + std::to_string(alpha) + ",0,true,null],[1],[]]");
}
Value Linear() {
  auto wire = Solid();
  wire.GetProperty(5u).Array()->set(
      0, Parse("[[2,32,-8,-8,32,[2164195328,4278255360,4278190335,1073742079],"
               "[0,0.5,0.5,1],0,[1,0,0,1,0,0]],[0.5,0,true,null],[1],[]]"));
  return wire;
}
struct Snapshot {
  std::vector<uint8_t> items, data;
  size_t boxes;
  bool clip;
  explicit Snapshot(const DisplayListBuilder& builder)
      : boxes(builder.BoxModelCount()),
        clip(builder.CurrentDisplayList().RootNeedClipBounds()) {
    const auto& list = builder.CurrentDisplayList();
    if (list.GetContentItemsByteSize())
      items.assign(list.GetContentItemsData(),
                   list.GetContentItemsData() + list.GetContentItemsByteSize());
    if (list.GetContentDataSize())
      data.assign(list.GetContentData(),
                  list.GetContentData() + list.GetContentDataSize());
  }
  void ExpectUnchanged(const DisplayListBuilder& builder) const {
    const Snapshot after(builder);
    EXPECT_EQ(after.items, items);
    EXPECT_EQ(after.data, data);
    EXPECT_EQ(after.boxes, boxes);
    EXPECT_EQ(after.clip, clip);
  }
};
DisplayList Sentinel() {
  DisplayListBuilder builder;
  Prefix(&builder);
  return builder.Build();
}
void ExpectSentinel(const DisplayList& output) {
  const auto expected = Sentinel();
  ASSERT_EQ(output.GetContentItemsByteSize(),
            expected.GetContentItemsByteSize());
  ASSERT_EQ(output.GetContentDataSize(), expected.GetContentDataSize());
  EXPECT_EQ(
      std::memcmp(output.GetContentItemsData(), expected.GetContentItemsData(),
                  expected.GetContentItemsByteSize()),
      0);
  EXPECT_EQ(std::memcmp(output.GetContentData(), expected.GetContentData(),
                        expected.GetContentDataSize()),
            0);
}
class ComposeDrawRecordingPolyfillTest : public ::testing::Test {
 protected:
  std::optional<DrawRecordingError> Compile(
      const Value& wire, const DisplayListBuilder& builder, DisplayList* output,
      const detail::DecodeLimits& limits = {}, double pixels = 1,
      ComposeDrawPhase phase = ComposeDrawPhase::kBehind,
      bool* effect = nullptr) {
    const Snapshot prefix(builder);
    bool shadow = true;
    const auto error = detail::DecodeDrawDisplayList(
        wire, pixels, phase, limits, builder, output, &shadow);
    prefix.ExpectUnchanged(builder);
    if (error) EXPECT_TRUE(shadow);
    if (effect) *effect = shadow;
    return error;
  }
};

TEST_F(ComposeDrawRecordingPolyfillTest,
       AlphaFoldsOnceAndPreparedOutputOwnsValues) {
  auto wire = Solid();
  DisplayListBuilder builder;
  Prefix(&builder);
  DisplayList compiled;
  ASSERT_FALSE(Compile(wire, builder, &compiled));
  wire = {};
  ASSERT_TRUE(builder.AppendDrawingContent(compiled));
  const auto list = builder.Build();
  DisplayListReader reader(list);
  int fills = 0;
  while (reader.HasNext()) {
    const auto& item = reader.Next();
    if (item.type != DisplayListOpType::kFill) continue;
    ++fills;
    EXPECT_EQ(item.payload.fill.color, 0x40123456u);
    EXPECT_EQ(item.payload.fill.clip_index, 1);
  }
  EXPECT_EQ(fills, 1);
  EXPECT_EQ(builder.BoxModelCount(), 2u);
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       SolidHoleUsesNativeBorderWithoutOverpaint) {
  DisplayListBuilder builder;
  DisplayList compiled;
  ASSERT_FALSE(Compile(Solid(.5, true), builder, &compiled));
  ASSERT_TRUE(builder.AppendDrawingContent(compiled));
  const auto list = builder.Build();
  DisplayListReader reader(list);
  int borders = 0, fills = 0, boxes = 0;
  while (reader.HasNext()) {
    const auto& item = reader.Next();
    if (item.type == DisplayListOpType::kRecordBox) ++boxes;
    if (item.type == DisplayListOpType::kFill) ++fills;
    if (item.type != DisplayListOpType::kBorder) continue;
    ++borders;
    EXPECT_EQ(item.payload.border.out_index, 0);
    EXPECT_EQ(item.payload.border.inner_index, 1);
    for (unsigned i = 0; i < 4; ++i) {
      EXPECT_EQ(item.payload.border.colors[i], 0x40123456u);
      EXPECT_EQ(item.payload.border.styles[i],
                static_cast<int32_t>(starlight::BorderStyleType::kSolid));
    }
  }
  EXPECT_EQ(boxes, 2);
  EXPECT_EQ(borders, 1);
  EXPECT_EQ(fills, 0);
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       ReversedOffboundsGradientPreservesHardStops) {
  DisplayListBuilder builder;
  DisplayList compiled;
  ASSERT_FALSE(Compile(Linear(), builder, &compiled));
  DisplayListReader reader(compiled);
  ASSERT_EQ(reader.Remaining(), 3u);
  reader.Next();  // Clip precedes the gradient tile.
  const auto& tile = reader.Next();
  ASSERT_EQ(tile.type, DisplayListOpType::kRecordBox);
  EXPECT_FLOAT_EQ(tile.payload.record_box.x, -8);
  EXPECT_FLOAT_EQ(tile.payload.record_box.y, -8);
  EXPECT_FLOAT_EQ(tile.payload.record_box.w, 48);
  EXPECT_FLOAT_EQ(tile.payload.record_box.h, 40);
  const auto& gradient = reader.Next();
  ASSERT_EQ(gradient.type, DisplayListOpType::kLinearGradient);
  const auto angle = gradient.payload.linear_gradient.angle;
  EXPECT_NEAR(angle, 225, .00001);
  ASSERT_EQ(gradient.payload.linear_gradient.color_count, 6u);
  const auto* colors = reader.Colors(gradient);
  const auto* stops = reader.Stops(gradient);
  EXPECT_EQ(colors[0], 0x40ff0000u);
  EXPECT_EQ(colors[2], 0x8000ff00u);
  EXPECT_EQ(colors[3], 0x800000ffu);
  EXPECT_EQ(colors[5], 0x200000ffu);
  EXPECT_FLOAT_EQ(stops[0], 0);
  EXPECT_FLOAT_EQ(stops[5], 1);
  EXPECT_FLOAT_EQ(stops[2], stops[3]);
  EXPECT_NE(colors[2], colors[3]);
  const double rad = angle * std::acos(-1.0) / 180;
  const double dx = std::sin(rad), dy = -std::cos(rad);
  const double span = std::abs(48 * dx) + std::abs(40 * dy);
  for (const auto point :
       {std::pair<double, double>{0, 0}, {40, 20}, {16, 12}}) {
    const double projected =
        ((point.first - 16) * dx + (point.second - 12) * dy) / span + .5;
    const double actual = (projected - stops[1]) / (stops[4] - stops[1]);
    const double expected =
        ((point.first - 32) * -40 + (point.second + 8) * 40) / 3200;
    EXPECT_NEAR(actual, expected, .000001);
  }
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       AdvisoryFeaturesDoNotGateEmptyOrZeroAlphaPreparation) {
  for (bool empty : {false, true}) {
    auto wire = Solid(0);
    if (empty) wire.GetProperty(4u).GetProperty(0u).Array()->set(3, Value(0));
    for (const char* declarations :
         {"[]", "[\"unknown.feature\"]",
          "[\"\",\"unknown.feature\",\"unknown.feature\"]",
          "[\"path.rect\",\"paint.solid\"]"}) {
      wire.Array()->set(7, Parse(declarations));
      DisplayListBuilder builder;
      Prefix(&builder);
      DisplayList compiled;
      ASSERT_FALSE(Compile(wire, builder, &compiled));
      if (empty)
        EXPECT_EQ(compiled.GetContentItemsSize(), 0u);
      else {
        DisplayListReader reader(compiled);
        ASSERT_EQ(reader.Remaining(), 2u);
        EXPECT_EQ(reader.Next().type, DisplayListOpType::kRecordBox);
        EXPECT_EQ(reader.Next().payload.fill.color, 0x00123456u);
      }
    }
  }
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       UnsupportedCapabilitiesRejectBeforeEmptyOrAlphaElision) {
  for (int family = 0; family < 19; ++family)
    for (bool empty : {false, true}) {
      SCOPED_TRACE(family);
      SCOPED_TRACE(empty);
      auto wire = Solid(0);
      const char* paths = nullptr;
      const char* paint = nullptr;
      const char* commands = nullptr;
      switch (family) {
        case 0:
          paths = "[[4,0,[[1,0,0],[2,40,0],[2,40,20],[2,0,20],[6]]]]";
          break;
        case 1:
          paths = "[[2,0,0,40,20,2,3,2,3,2,3,2,3]]";
          break;
        case 2:
          paint = "[[1,1],[0,0,true,null],[2,0,0,0,4,[],0,null],[]]";
          break;
        case 3:
          paint = "[[1,1],[0,0,true,[1,0,0]],[1],[]]";
          break;
        case 4:
          paint = "[[1,1],[0,1,true,null],[1],[]]";
          break;
        case 5:
          paint = "[[1,1],[0,0,false,null],[1],[]]";
          break;
        case 6:
          paint = "[[1,1],[0,0,true,null],[1],[[255,\"effect\",1,[],[]]]]";
          break;
        case 7:
          paint =
              "[[3,0,0,1,[1,2],[0,1],0,[1,0,0,1,0,0]],[0,0,true,null],[1],[]]";
          break;
        case 8:
          commands = "[[1,0,0],[4,1,0,0,1,1,0]]";
          break;
        case 9:
          commands = "[[1,0,0],[5,0,0,true]]";
          break;
        case 10:
          commands = "[[1,0,0],[6,[0,0,1,1],0],[7]]";
          break;
        case 11:
          commands = "[[10]]";
          break;
        case 12:
          wire.Array()->set(6, Parse("[[\"unknown.resource\",1,[]]]"));
          break;
        case 13:
          paint =
              "[[2,32,-8,-8,32,[1,2],[0,1],1,[1,0,0,1,0,0]],[0,0,true,null],[1]"
              ",[]]";
          break;
        case 14:
          paint =
              "[[2,32,-8,-8,32,[1,2],[0,1],0,[1,0,0,1,1,0]],[0,0,true,null],[1]"
              ",[]]";
          break;
        case 15:
          commands = "[[6,[0,0,1,1],0],[7]]";
          break;
        case 16:
          commands = "[[9,0,0,0,0,1,0,0,0]]";
          break;
        case 17:
          commands = "[[9,0,0,0,0,1,0,1,1]]";
          break;
        case 18:
          paths = "[[1,0,0,40,20],[1,2,3,36,15],[3,0,1]]";
          commands = "[[1,2,0]]";
          break;
      }
      if (paths) wire.Array()->set(4, Parse(paths));
      if (paint) wire.GetProperty(5u).Array()->set(0, Parse(paint));
      if (commands) wire.Array()->set(3, Parse(commands));
      if (empty && family != 0)
        wire.GetProperty(4u).GetProperty(0u).Array()->set(3, Value(0));
      if (empty && family == 0)
        wire.Array()->set(
            4, Parse("[[4,0,[[1,0,0],[2,0,0],[2,0,20],[2,0,20],[6]]]]"));
      DisplayListBuilder builder;
      Prefix(&builder);
      auto output = Sentinel();
      const auto error = Compile(wire, builder, &output);
      ASSERT_TRUE(error);
      EXPECT_EQ(error->code, Code::kUnsupported);
      ExpectSentinel(output);
    }
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       LateInvalidAndDerivedOverflowLeaveNonemptyPrefixExact) {
  for (int tail = 0; tail < 6; ++tail) {
    auto wire = Solid();
    wire.GetProperty(4u).Array()->push_back(Parse("[1,0,0,40,20]"));
    wire.GetProperty(3u).Array()->push_back(Parse("[1,1,0]"));
    switch (tail) {
      case 0:
        wire.GetProperty(4u).GetProperty(1u).Array()->set(3, Value(-1));
        break;
      case 1:
        wire.GetProperty(4u).GetProperty(1u).Array()->set(1, Value(1e40));
        break;
      case 2:
        wire.GetProperty(3u).Array()->set(1, Parse("[1,999,0]"));
        break;
      case 3:
        wire.GetProperty(3u).Array()->set(1,
                                          Parse("[255,\"unknown\",1,[],[]]"));
        break;
      case 4:
        wire.GetProperty(3u).Array()->set(1,
                                          Parse("[9,1,0,3e38,0,2,3e38,0,1]"));
        break;
      case 5:
        wire.GetProperty(5u).Array()->push_back(
            Parse("[[2,-3e38,0,3e38,0,[1,2],[0,1],0,[1,0,0,1,0,0]],[1,0,true,"
                  "null],[1],[]]"));
        wire.GetProperty(3u).Array()->set(1, Parse("[1,1,1]"));
        break;
    }
    DisplayListBuilder builder;
    Prefix(&builder);
    auto output = Sentinel();
    ASSERT_TRUE(Compile(wire, builder, &output));
    ExpectSentinel(output);
  }
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       RemainingCapacityIncludesPrefixAndExpandedStops) {
  auto wire = Linear();
  wire.GetProperty(3u).Array()->push_back(Parse("[1,0,0]"));
  for (int budget = 0; budget < 4; ++budget) {
    DisplayListBuilder builder;
    Prefix(&builder);
    detail::DecodeLimits limits;
    if (budget == 0) limits.max_items = 8;
    if (budget == 1) limits.max_boxes = 4;
    if (budget == 2) limits.max_data_bytes = 111;
    if (budget == 3) limits.max_gradient_stops = 5;
    auto output = Sentinel();
    const auto error = Compile(wire, builder, &output, limits);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, Code::kCapacity);
    ExpectSentinel(output);
  }
  DisplayListBuilder builder;
  Prefix(&builder);
  DisplayList output;
  detail::DecodeLimits limits;
  limits.max_items = 9;
  limits.max_data_bytes = 112;
  limits.max_boxes = 5;
  limits.max_gradient_stops = 6;
  ASSERT_FALSE(Compile(wire, builder, &output, limits));
  ASSERT_TRUE(builder.AppendDrawingContent(output));
  EXPECT_EQ(builder.CurrentDisplayList().GetContentItemsSize(), 9u);
  EXPECT_EQ(builder.CurrentDisplayList().GetContentDataSize(), 112u);
  EXPECT_EQ(builder.BoxModelCount(), 5u);
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       V1PhysicalUniformHoleUsesPhysicalGeometryAtLargeOrigins) {
  auto wire = Solid(1, true);
  wire.GetProperty(4u).GetProperty(0u).Array()->set(1, Value(1000000));
  wire.GetProperty(4u).GetProperty(1u).Array()->set(1, Value(1000002));
  for (double pixels : {1, 2, 3}) {
    DisplayListBuilder builder;
    DisplayList output;
    ASSERT_FALSE(Compile(wire, builder, &output, {}, pixels));
    ASSERT_EQ(output.GetContentItemsSize(), 3u);
    DisplayListReader reader(output);
    EXPECT_FLOAT_EQ(reader.Next().payload.record_box.x, 1000000 / pixels);
    EXPECT_DOUBLE_EQ(
        wire.GetProperty(4u).GetProperty(0u).GetProperty(1u).Number(), 1000000);
  }
  wire.GetProperty(4u).GetProperty(1u).Array()->set(2, Value(3));
  DisplayListBuilder builder;
  DisplayList output;
  const auto error = Compile(wire, builder, &output, {}, 3);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kUnsupported);
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       EmptyModifierClipAndBalancedIdentityStateRemainDistinct) {
  auto wire = Wire("[[5,0,0,true]]", "[[1,0,0,0,0]]");
  wire.Array()->set(5, Parse("[]"));
  DisplayListBuilder builder;
  DisplayList output;
  ASSERT_FALSE(Compile(wire, builder, &output, {}, 1, ComposeDrawPhase::kClip));
  DisplayListReader reader(output);
  ASSERT_EQ(reader.Remaining(), 1u);
  const auto& clip = reader.Next();
  EXPECT_EQ(clip.type, DisplayListOpType::kClipRect);
  EXPECT_FLOAT_EQ(clip.payload.clip_rect.w, 0);
  EXPECT_FLOAT_EQ(clip.payload.clip_rect.h, 0);
  EXPECT_TRUE(Compile(wire, builder, &output));
  wire.Array()->set(3, Parse("[[2],[4,1,0,0,1,0,0],[3]]"));
  ASSERT_FALSE(Compile(wire, builder, &output));
  EXPECT_EQ(output.GetContentItemsSize(), 0u);
  wire.Array()->set(3, Parse("[[3]]"));
  EXPECT_TRUE(Compile(wire, builder, &output));
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       LiteralV1ShadowCoverageIsCheckedBeforePublication) {
  struct Case {
    const char* wire;
    bool overflow;
    bool empty;
  };
  const Case cases[] = {
      // The exact independently demonstrated wire: both radius divisions Inf.
      {R"json(["lynx.draw.recording",1,0,[[9,0,0,0,0,1,1,0,1]],[[2,0,0,1.401298464324817e-45,1.401298464324817e-45,1,1,1,1,1,1,1,1]],[[[1,2148676694],[1,0,true,null],[1],[]]],[],["drawing.shadow","paint.blend.SrcOver","paint.solid","paint.srgb","path.roundrect"],[]])json",
       true, false},
      // Finite quotients, but doubling their minimum overflows.
      {R"json(["lynx.draw.recording",1,0,[[9,0,0,0,0,1,1,0,1]],[[2,0,0,1,1,2e38,2e38,2e38,2e38,2e38,2e38,2e38,2e38]],[[[1,2148676694],[1,0,true,null],[1],[]]],[],["drawing.shadow","paint.blend.SrcOver","paint.solid","paint.srgb","path.roundrect"],[]])json",
       true, false},
      // min() hides a nonfinite x quotient behind a finite y quotient.
      {R"json(["lynx.draw.recording",1,0,[[9,0,0,0,0,1,1,0,1]],[[2,0,0,1.401298464324817e-45,10,1,1,1,1,1,1,1,1]],[[[1,2148676694],[1,0,true,null],[1],[]]],[],["drawing.shadow","paint.blend.SrcOver","paint.solid","paint.srgb","path.roundrect"],[]])json",
       true, false},
      // The final corner must be checked too, not just the first coverage.
      {R"json(["lynx.draw.recording",1,0,[[9,0,0,0,0,1,1,0,1]],[[2,0,0,1.401298464324817e-45,1.401298464324817e-45,0,0,0,0,0,0,1,1]],[[[1,2148676694],[1,0,true,null],[1],[]]],[],["drawing.shadow","paint.blend.SrcOver","paint.solid","paint.srgb","path.roundrect"],[]])json",
       true, false},
      {R"json(["lynx.draw.recording",1,0,[[9,0,0,0,0,1,1,0,1]],[[2,0,0,10,10,1,1,1,1,1,1,1,1]],[[[1,2148676694],[1,0,true,null],[1],[]]],[],["drawing.shadow","paint.blend.SrcOver","paint.solid","paint.srgb","path.roundrect"],[]])json",
       false, false},
      // ComputeShadowBox does not execute radius/edge division for an empty
      // box.
      {R"json(["lynx.draw.recording",1,0,[[9,0,0,0,0,1,1,0,1]],[[2,0,0,0,20,1,1,1,1,1,1,1,1]],[[[1,2148676694],[1,0,true,null],[1],[]]],[],["drawing.shadow","paint.blend.SrcOver","paint.solid","paint.srgb","path.roundrect"],[]])json",
       false, true},
  };
  for (const auto& test : cases)
    for (bool flag : {false, true}) {
      DisplayListBuilder builder;
      Prefix(&builder);
      auto output = Sentinel();
      bool shadow = flag;
      const auto error = detail::DecodeDrawDisplayList(
          Parse(test.wire), 1, ComposeDrawPhase::kBehind, {}, builder, &output,
          &shadow);
      EXPECT_EQ(static_cast<bool>(error), test.overflow);
      if (test.overflow) {
        ASSERT_TRUE(error);
        EXPECT_EQ(error->code, Code::kOverflow);
        EXPECT_EQ(shadow, flag);
        ExpectSentinel(output);
      } else if (test.empty) {
        EXPECT_EQ(output.GetContentItemsSize(), 0u);
        EXPECT_FALSE(shadow);
      } else {
        EXPECT_TRUE(shadow);
        DisplayListReader reader(output);
        ASSERT_EQ(reader.Remaining(), 3u);
        reader.Next();
        const auto& box = reader.Next();
        EXPECT_EQ(box.type, DisplayListOpType::kRecordBox);
        EXPECT_FLOAT_EQ(box.payload.record_box.x, -1);
        EXPECT_FLOAT_EQ(box.payload.record_box.y, -1);
        EXPECT_FLOAT_EQ(box.payload.record_box.w, 12);
        EXPECT_FLOAT_EQ(box.payload.record_box.h, 12);
        EXPECT_FLOAT_EQ(box.payload.record_box.radii[0], 2);
        const auto& item = reader.Next();
        EXPECT_EQ(item.type, DisplayListOpType::kBoxShadow);
        EXPECT_EQ(item.payload.box_shadow.color, 0x80123456u);
        EXPECT_FLOAT_EQ(item.payload.box_shadow.blur_radius, 1);
      }
      EXPECT_EQ(builder.BoxModelCount(), 1u);
    }
}
TEST_F(ComposeDrawRecordingPolyfillTest,
       LiteralV1EmptyLinearChecksDerivedBeforeElision) {
  const char* overflow =
      R"json(["lynx.draw.recording",1,0,[[1,0,0]],[[1,0,0,0,20]],[[[2,-3e38,0,3e38,0,[4279383126,4289449455],[0,1],0,[1,0,0,1,0,0]],[1,0,true,null],[1],[]]],[],["paint.blend.SrcOver","paint.linear","paint.srgb","paint.tile.Clamp","path.rect"],[]])json";
  const char* finite =
      R"json(["lynx.draw.recording",1,0,[[1,0,0]],[[1,0,0,0,20]],[[[2,-3,0,3,0,[4279383126,4289449455],[0,1],0,[1,0,0,1,0,0]],[1,0,true,null],[1],[]]],[],["paint.blend.SrcOver","paint.linear","paint.srgb","paint.tile.Clamp","path.rect"],[]])json";
  for (const char* text : {overflow, finite})
    for (double width : {0., 1.}) {
      if (text == finite && width != 0) continue;
      auto wire = Parse(text);
      wire.GetProperty(4u).GetProperty(0u).Array()->set(3, Value(width));
      for (bool flag : {false, true}) {
        DisplayListBuilder builder;
        Prefix(&builder);
        const Snapshot prefix(builder);
        auto output = Sentinel();
        bool shadow = flag;
        detail::DecodeLimits limits;
        if (text == finite) {
          limits.max_items = builder.CurrentDisplayList().GetContentItemsSize();
          limits.max_data_bytes =
              builder.CurrentDisplayList().GetContentDataSize();
          limits.max_boxes = builder.BoxModelCount();
          limits.max_gradient_stops = 4;
        }
        const auto error =
            detail::DecodeDrawDisplayList(wire, 1, ComposeDrawPhase::kBehind,
                                          limits, builder, &output, &shadow);
        EXPECT_EQ(static_cast<bool>(error), text == overflow);
        if (error) {
          EXPECT_EQ(error->code, Code::kOverflow);
          ExpectSentinel(output);
          EXPECT_EQ(shadow, flag);
        } else {
          EXPECT_EQ(output.GetContentItemsSize(), 0u);
          EXPECT_FALSE(shadow);
        }
        prefix.ExpectUnchanged(builder);
        RoundedRectangle next_box;
        int32_t next;
        builder.RecordBoxModel(next_box, next);
        EXPECT_EQ(next, 1);
      }
    }
}
}  // namespace
}  // namespace lynx::tasm::testing
