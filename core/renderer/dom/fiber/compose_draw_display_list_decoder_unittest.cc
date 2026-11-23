// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <iostream>
#include <limits>

#include "base/include/vector.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fiber/testing/compose_draw_recording_contract_test_helpers.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "gfx/public/display_list_reader.h"

namespace lynx::tasm::testing {
namespace {
using namespace contract_fixture;
using Code = DrawRecordingError::Code;
using Value = lepus::Value;

Value Parse(const std::string& text) {
  rapidjson::Document d;
  d.Parse(text.c_str());
  EXPECT_FALSE(d.HasParseError());
  return Json(d);
}
std::string Wire(const std::string& commands = "[[1,0,0]]",
                 const std::string& paths = "[[1,0,0,40,20]]",
                 const std::string& metadata = "[]") {
  return "[\"lynx.draw.recording\",1,0," + commands + "," + paths +
         ",[[[1,2148676694],[0.5,0,true,null],[1],[]]],[],[]," + metadata + "]";
}
struct Bytes {
  base::ByteArray items, data;
  explicit Bytes(const DisplayList& list)
      : items(list.GetContentItemsByteSize(), list.GetContentItemsData()),
        data(list.GetContentDataSize(), list.GetContentData()) {}
  void Expect(const DisplayList& list) const {
    const Bytes after(list);
    EXPECT_EQ(after.items, items);
    EXPECT_EQ(after.data, data);
  }
};
class DrawDisplayListDecoderTest : public ::testing::Test {
 protected:
  Value Eval(const std::string& script) {
    if (!runtime_) {
      runtime_ = runtime::MTSRuntime::CreateContext(
          runtime::ContextType::LepusNGContextType);
      runtime_->Initialize();
    }
    Value value;
    EXPECT_TRUE(runtime::MTSRuntime::ToQuickContext(runtime_.get())
                    ->EvalBuf(script.data(), script.size(), value,
                              "display-list-decoder.js"));
    return value;
  }
  std::optional<DrawRecordingError> Run(
      const char* id, const Value& wire, double pixels = 1,
      ComposeDrawPhase phase = ComposeDrawPhase::kBehind) {
    DisplayListBuilder sentinel;

    Prefix(&sentinel);

    output_ = sentinel.Build();
    const Bytes output_before(output_);
    shadow_ = false;
    const auto error =
        detail::DecodeDrawDisplayList(wire, pixels, phase, &output_, &shadow_);

    if (error) {
      output_before.Expect(output_);
      EXPECT_FALSE(shadow_);
    }
    std::cout << "DIRECT {\"id\":\"" << id << "\",\"pixels\":" << pixels
              << ",\"error_code\":"
              << (error ? static_cast<int>(error->code) : 0)
              << ",\"command_index\":";
    if (error && error->command_index != DrawRecordingError::kNoCommand)
      std::cout << error->command_index;
    else
      std::cout << "null";
    std::cout << ",\"namespace\":\"" << (error ? error->name_space : "")
              << "\",\"shadow\":" << shadow_
              << ",\"output_items\":" << output_.GetContentItemsSize()
              << ",\"output_data\":" << output_.GetContentDataSize() << "}"
              << std::endl;
    return error;
  }
  void Reject(const char* id, const Value& wire, Code code,
              size_t command = DrawRecordingError::kNoCommand,
              const char* name_space = "",
              ComposeDrawPhase phase = ComposeDrawPhase::kBehind) {
    for (double pixels : {1., 2., 3.}) {
      SCOPED_TRACE(id);
      const auto error = Run(id, wire, pixels, phase);
      ASSERT_TRUE(error);
      EXPECT_EQ(error->code, code);
      EXPECT_EQ(error->command_index, command);
      EXPECT_EQ(error->name_space, name_space);
    }
  }
  decltype(runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType)) runtime_;
  DisplayList output_;
  bool shadow_{false};
};

TEST_F(DrawDisplayListDecoderTest,
       InvalidPixelScalesPreserveOutputAndShadowFlag) {
  const auto wire = Parse(Wire());
  for (double pixels : {0., -1., std::numeric_limits<double>::quiet_NaN(),
                        std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity()}) {
    SCOPED_TRACE(pixels);
    DisplayList output;
    bool shadow = false;
    const auto error = detail::DecodeDrawDisplayList(
        wire, pixels, ComposeDrawPhase::kBehind, &output, &shadow);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, Code::kMalformed);
    EXPECT_EQ(error->command_index, DrawRecordingError::kNoCommand);
    EXPECT_EQ(output.GetContentItemsSize(), 0u);
    EXPECT_EQ(output.GetContentDataSize(), 0u);
    EXPECT_FALSE(shadow);
  }
}

TEST_F(DrawDisplayListDecoderTest, CompilesIndependentFinalStorage) {
  DisplayList output;
  bool shadow = false;
  ASSERT_FALSE(detail::DecodeDrawDisplayList(
      Parse(Wire()), 2, ComposeDrawPhase::kBehind, &output, &shadow));
  EXPECT_FALSE(shadow);

  EXPECT_EQ(output.GetContentItemsSize(), 2u);
  DisplayListReader reader(output);
  ASSERT_TRUE(reader.HasNext());
  const auto& box = reader.Next();
  ASSERT_EQ(box.type, DisplayListOpType::kRecordBox);
  EXPECT_FLOAT_EQ(box.payload.record_box.w, 20);
  EXPECT_FLOAT_EQ(box.payload.record_box.h, 10);
  ASSERT_TRUE(reader.HasNext());
  const auto& fill = reader.Next();
  ASSERT_EQ(fill.type, DisplayListOpType::kFill);
  EXPECT_EQ(fill.payload.fill.clip_index, 0);
}
TEST_F(DrawDisplayListDecoderTest, DemandConversionAndPhysicalDoubleInset) {
  for (double pixels : {1., 2., 3.}) {
    const std::string paths = "[[1,0,0,40,20],[1,1e40,0,40,20]]";
    ASSERT_FALSE(
        Run("unused-float-overflow", Parse(Wire("[[1,0,0]]", paths)), pixels));
    EXPECT_EQ(output_.GetContentItemsSize(), 2u);
    const auto overflow = Run("referenced-float-overflow",
                              Parse(Wire("[[1,0,0],[1,1,0]]", paths)), pixels);
    ASSERT_TRUE(overflow);
    EXPECT_EQ(overflow->code, Code::kOverflow);
    EXPECT_EQ(overflow->command_index, 1u);
    ASSERT_FALSE(Run("double-uniform-inset",
                     Parse(Wire("[[1,2,0]]",
                                "[[1,10000000000,10000000000,40,20],"
                                "[1,10000000002,10000000002,36,16],[3,0,1]]")),
                     pixels));
    EXPECT_EQ(output_.GetContentItemsSize(), 3u);
  }
  Reject("double-nonuniform-inset",
         Parse(Wire("[[1,2,0]]",
                    "[[1,10000000000,10000000000,40,20],"
                    "[1,10000000002,10000000003,36,16],[3,0,1]]")),
         Code::kUnsupported);
}

TEST_F(DrawDisplayListDecoderTest, SolidBorderDoesNotRequireGradientStorage) {
  const auto wire = Parse(Wire("[[1,2,0]]",
                               "[[2,0,0,40,20,8,8,8,8,8,8,8,8],"
                               "[2,2,2,36,16,6,6,6,6,6,6,6,6],[3,0,1]]"));
  ASSERT_FALSE(Run("solid-border", wire));
  EXPECT_EQ(output_.GetContentItemsSize(), 3u);
  EXPECT_EQ(output_.GetContentDataSize(), 0u);
}

TEST_F(DrawDisplayListDecoderTest, SemanticAndCapabilityPrecedence) {
  const std::string ellipse = "[[2,0,0,40,20,12,4,12,4,12,4,12,4]]";
  Reject("late-metadata-before-path",
         Parse(Wire("[[1,0,0]]", ellipse, "[[42,0]]")), Code::kMalformed);
  Reject("late-metadata-before-lowering",
         Parse(Wire("[[1,0,0]]", "[[1,1e40,0,40,20]]", "[[42,0]]")),
         Code::kMalformed);
  auto wire = Parse(Wire("[[1,0,0]]", ellipse));
  wire.GetProperty(5u).Array()->push_back(
      Parse("[[3,0,0,1,[1,2],[0,1],0,[1,0,0,1,0,0]],[1,0,true,null],[1],[]]"));
  Reject("path-before-paint-and-phase", wire, Code::kUnsupported,
         DrawRecordingError::kNoCommand, "", ComposeDrawPhase::kClip);
  wire.GetProperty(4u).Array()->set(0, Parse("[1,0,0,40,20]"));
  Reject("paint-before-phase", wire, Code::kUnsupported,
         DrawRecordingError::kNoCommand, "paint.radial",
         ComposeDrawPhase::kClip);
  wire.GetProperty(5u).Array()->set(1, Parse("[[1,1],[1,0,true,null],[1],[]]"));
  Reject("phase-before-command", wire, Code::kUnsupported,
         DrawRecordingError::kNoCommand, "", ComposeDrawPhase::kClip);
  auto unknown = Parse(Wire("[[255,\"unknown\",1,[],[]]]"));
  unknown.Array()->set(7, Value(42));
  Reject("unknown-before-features", unknown, Code::kUnsupported,
         DrawRecordingError::kNoCommand, "unknown");
  auto unmatched = Parse(Wire("[[2]]", "[[1,0,0,40,20]]", "[[42,0]]"));
  Reject("unmatched-before-metadata", unmatched, Code::kMalformed, 0);
}

TEST_F(DrawDisplayListDecoderTest, ExactBoundaryCapabilityAndMetadataInputs) {
  const auto wire = Wire("[[1,0,0]]", "[[2,0,0,40,20,12,4,12,4,12,4,12,4]]");
  Reject("elliptical-capability", Parse(wire), Code::kUnsupported);
  Reject("late-malformed-metadata",
         Parse(Wire("[[1,0,0]]", "[[2,0,0,40,20,12,4,12,4,12,4,12,4]]",
                    "[[42,0]]")),
         Code::kMalformed);
}

TEST_F(DrawDisplayListDecoderTest, UnsupportedUnusedRowsStillValidate) {
  for (const auto* paint :
       {"[[2,0,0,10,10,[1,2,3],[0,0.75,0.5],0,[1,0,0,1,0,0]],[1,0,true,null],["
        "1],[]]",
        "[[3,0,0,1,[1,2],[0,1],0,[1,0,0,1,0,null]],[1,0,true,null],[1],[]]",
        "[[1,1],[1,0,true,null],[2,2,0,0,4,[1,2,3,0],0,null],[]]",
        "[[1,1],[1,0,true,null],[1],[[255,\"effect\",1,[\"text\",[1,2]],[]],"
        "[255,\"effect\",1,[\"text\",{}],[]]]]"}) {
    auto wire = Parse(Wire());
    wire.GetProperty(5u).Array()->push_back(Parse(paint));
    Reject("late-invalid-unused-paint", wire, Code::kMalformed);
  }
  for (const auto* path :
       {"[3,1,-1]", "[3,2,-1]", "[3,0,1]", "[3,0,2]", "[255,\"path\",1,[],[1]]",
        "[255,\"path\",1,[],[2]]"}) {
    auto wire = Parse(Wire());
    wire.GetProperty(4u).Array()->push_back(Parse(path));
    wire.GetProperty(4u).Array()->push_back(Parse("[1,0,0,10,10]"));
    Reject("self-or-future-ref", wire, Code::kMalformed);
  }
  auto wire = Parse(Wire());
  wire.GetProperty(4u).Array()->push_back(Parse("[4,0,[[1,0,0],[2,1,1],[6]]]"));
  Reject("unused-valid-verbs", wire, Code::kUnsupported);
  wire.GetProperty(4u).Array()->set(1, Parse("[4,0,[[2,1,1]]]"));
  Reject("unused-invalid-verbs", wire, Code::kMalformed);
}

TEST_F(DrawDisplayListDecoderTest, MultipleGradientsOwnExpandedStops) {
  auto wire = Parse(Wire("[[1,0,0],[1,0,0]]"));
  wire.GetProperty(5u).Array()->set(
      0, Parse("[[2,32,-8,-8,32,[2164195328,4278255360,4278190335,1073742079],"
               "[0,0.5,0.5,1],0,[1,0,0,1,0,0]],[0.5,0,true,null],[1],[]]"));
  ASSERT_FALSE(Run("expanded-stops", wire));
  EXPECT_EQ(output_.GetContentItemsSize(), 6u);
  EXPECT_EQ(output_.GetContentDataSize(), 96u);
  DisplayListReader reader(output_);
  unsigned gradients = 0;
  while (reader.HasNext()) {
    const auto& item = reader.Next();
    if (item.type != DisplayListOpType::kLinearGradient) continue;
    ++gradients;
    EXPECT_FLOAT_EQ(item.payload.linear_gradient.angle, 225);
    ASSERT_EQ(item.payload.linear_gradient.color_count, 6u);
    EXPECT_EQ(reader.Colors(item)[0], 0x40ff0000u);
    EXPECT_EQ(reader.Colors(item)[2], 0x8000ff00u);
    EXPECT_EQ(reader.Colors(item)[3], 0x800000ffu);
    EXPECT_EQ(reader.Colors(item)[5], 0x200000ffu);
    EXPECT_FLOAT_EQ(reader.Stops(item)[0], 0);
    EXPECT_FLOAT_EQ(reader.Stops(item)[2], reader.Stops(item)[3]);
    EXPECT_FLOAT_EQ(reader.Stops(item)[5], 1);
  }
  EXPECT_EQ(gradients, 2u);
}

TEST_F(DrawDisplayListDecoderTest, EmptyPrimitivesValidateBeforeElision) {
  for (const char* endpoints : {"-3,0,3,0", "-3e38,0,3e38,0"}) {
    auto wire = Parse(Wire("[[1,0,0]]", "[[1,0,0,0,20]]"));
    wire.GetProperty(5u).Array()->set(
        0, Parse(std::string("[[2,") + endpoints +
                 ",[1,2],[0,1],0,[1,0,0,1,0,0]],[1,0,true,null],[1],[]]"));
    const auto error = Run("empty-linear", wire);
    if (std::strstr(endpoints, "e38")) {
      ASSERT_TRUE(error);
      EXPECT_EQ(error->code, Code::kOverflow);
      EXPECT_EQ(error->command_index, 0u);
    } else {
      ASSERT_FALSE(error);
      EXPECT_EQ(output_.GetContentItemsSize(), 0u);
    }
  }
  auto clip = Parse(
      "[\"lynx.draw.recording\",1,0,[[5,0,0,true]],[[1,0,0,0,0]],[],[],[],[]]");
  ASSERT_FALSE(Run("empty-clip", clip, 1, ComposeDrawPhase::kClip));
  ASSERT_EQ(output_.GetContentItemsSize(), 1u);
  DisplayListReader reader(output_);
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kClipRect);
  ASSERT_FALSE(Run("identity-state", Parse(Wire("[[2],[4,1,0,0,1,0,0],[3]]"))));
  EXPECT_EQ(output_.GetContentItemsSize(), 0u);
  for (const char* geometry :
       {"1.401298464324817e-45,1.401298464324817e-45,1,1,1,1,1,1,1,1",
        "1,1,2e38,2e38,2e38,2e38,2e38,2e38,2e38,2e38"}) {
    const auto error =
        Run("shadow-derived-overflow",
            Parse(Wire("[[9,0,0,0,0,1,1,0,1]]",
                       std::string("[[2,0,0,") + geometry + "]]")));
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, Code::kOverflow);
    EXPECT_EQ(error->command_index, 0u);
  }
  for (double pixels : {2., 3.}) {
    ASSERT_FALSE(Run("shadow-underflow-empty",
                     Parse(Wire("[[9,0,0,0,0,1,1,0,1]]",
                                "[[2,0,0,1.401298464324817e-45,1."
                                "401298464324817e-45,1,1,1,1,1,1,1,1]]")),
                     pixels));
    EXPECT_EQ(output_.GetContentItemsSize(), 0u);
    EXPECT_FALSE(shadow_);
  }
  ASSERT_FALSE(Run(
      "empty-shadow",
      Parse(Wire("[[9,0,0,0,0,1,1,0,1]]", "[[2,0,0,0,20,1,1,1,1,1,1,1,1]]"))));
  EXPECT_FALSE(shadow_);
  EXPECT_EQ(output_.GetContentItemsSize(), 0u);
}

TEST_F(DrawDisplayListDecoderTest, CyclesRejectAndSharedDagRemainsValid) {
  auto cycle = Eval(
      "(()=>{let a=[];a.push(a);return "
      "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
  Reject("payload-cycle", cycle, Code::kMalformed);
  auto dag = Eval(
      "(()=>{let a=[];for(let i=0;i<3;i++)a=[a,a];return "
      "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
  ASSERT_FALSE(Run("shared-dag", dag));
  EXPECT_EQ(output_.GetContentItemsSize(), 0u);
}

}  // namespace
}  // namespace lynx::tasm::testing
