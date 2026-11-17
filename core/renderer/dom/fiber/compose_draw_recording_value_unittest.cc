// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>

#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fiber/testing/compose_draw_recording_contract_test_helpers.h"
#include "gfx/public/display_list_reader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm::testing {
namespace {

lepus::Value Parse(const char* text) {
  rapidjson::Document document;
  document.Parse(text);
  EXPECT_FALSE(document.HasParseError());
  return contract_fixture::Json(document);
}
TEST(ComposeDrawRecordingValueTest, DefaultsKeepIndependentAlphaAndHairline) {
  DisplayListBuilder destination;
  DisplayList output;
  bool shadow = true;
  auto wire = Parse(
      R"json(["lynx.draw.recording",1,0,[[1,0,0]],[[1,0,0,12,8]],[[[1,2148606515],[0.5,0,true,null],[1],[]]],[],[],[]])json");
  ASSERT_FALSE(detail::DecodeDrawDisplayList(
      wire, 1, ComposeDrawPhase::kBehind, {}, destination, &output, &shadow));
  DisplayListReader reader(output);
  ASSERT_EQ(reader.Remaining(), 2u);
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(reader.Next().payload.fill.color, 0x40112233u);
  EXPECT_FALSE(shadow);
  wire.GetProperty(5u).GetProperty(0u).Array()->set(
      2, Parse("[2,0,0,0,4,[],0,null]"));
  const auto error = detail::DecodeDrawDisplayList(
      wire, 1, ComposeDrawPhase::kBehind, {}, destination, &output, &shadow);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, DrawRecordingError::Code::kUnsupported);
  EXPECT_EQ(error->name_space, "paint.stroke");
}
TEST(ComposeDrawRecordingValueTest, PhysicalDoublesAndInputRadiiAreNotLowered) {
  auto wire = Parse(
      R"json(["lynx.draw.recording",1,0,[[1,2,0]],[[1,10000000000,10000000000,40,20],[1,10000000002,10000000002,36,16],[3,0,1]],[[[1,2148606515],[1,0,true,null],[1],[]]],[],[],[]])json");
  for (double pixels : {1., 2., 3.}) {
    DisplayListBuilder destination;
    DisplayList output;
    bool shadow = true;
    ASSERT_FALSE(detail::DecodeDrawDisplayList(wire, pixels,
                                               ComposeDrawPhase::kBehind, {},
                                               destination, &output, &shadow));
    EXPECT_EQ(output.GetContentItemsSize(), 3u);
    EXPECT_DOUBLE_EQ(
        wire.GetProperty(4u).GetProperty(1u).GetProperty(1u).Number(),
        10000000002.);
    EXPECT_FALSE(shadow);
  }
}
TEST(ComposeDrawRecordingValueTest, MalformedReferencesRemainRawOwnedValues) {
  for (
      const char* text :
      {R"json(["lynx.draw.recording",1,0,[],[[3,65537,-7]],[],[],[],[]])json",
       R"json(["lynx.draw.recording",1,0,[[1,-9,65538]],[],[],[],[],[]])json"}) {
    DisplayListBuilder destination;
    contract_fixture::Prefix(&destination);
    DisplayListBuilder sentinel;
    contract_fixture::Prefix(&sentinel);
    auto output = sentinel.Build();
    bool shadow = true;
    const auto error =
        detail::DecodeDrawDisplayList(Parse(text), 1, ComposeDrawPhase::kBehind,
                                      {}, destination, &output, &shadow);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, DrawRecordingError::Code::kMalformed);
    EXPECT_EQ(output.GetContentItemsSize(), 3u);
    EXPECT_EQ(destination.BoxModelCount(), 1u);
    EXPECT_TRUE(shadow);
  }
}

TEST(ComposeDrawRecordingValueTest,
     ErrorCopiesRetainCategoryIndexAndNamespace) {
  const std::array<DrawRecordingError::Code, 5> codes{
      DrawRecordingError::Code::kVersion, DrawRecordingError::Code::kMalformed,
      DrawRecordingError::Code::kUnsupported,
      DrawRecordingError::Code::kOverflow, DrawRecordingError::Code::kCapacity};
  for (size_t i = 0; i < codes.size(); ++i) {
    for (size_t j = i + 1; j < codes.size(); ++j) {
      EXPECT_NE(codes[i], codes[j]);
    }
    const auto code = codes[i];
    DrawRecordingError source{code, 23, "test.namespace"};
    const auto copy = source;
    source.name_space.clear();
    EXPECT_EQ(copy.code, code);
    EXPECT_EQ(copy.command_index, 23u);
    EXPECT_EQ(copy.name_space, "test.namespace");
  }
}

TEST(ComposeDrawRecordingValueTest,
     PrivateLimitsHaveCoreDefaultsAndIndependentCopies) {
  detail::DecodeLimits limits;
  EXPECT_EQ(limits.max_commands, 65536u);
  EXPECT_EQ(limits.max_paths, 65536u);
  EXPECT_EQ(limits.max_paints, 65536u);
  EXPECT_EQ(limits.max_resources, 4096u);
  EXPECT_EQ(limits.max_path_depth, 64u);
  EXPECT_EQ(limits.max_state_depth, 64u);
  EXPECT_EQ(limits.max_value_depth, 64u);
  EXPECT_EQ(limits.max_payload_bytes, 16777216u);
  auto injected = limits;
  injected.max_value_depth = 8;
  EXPECT_EQ(limits.max_value_depth, 64u);
  EXPECT_EQ(injected.max_value_depth, 8u);
}

}  // namespace
}  // namespace lynx::tasm::testing
