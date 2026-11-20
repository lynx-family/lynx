// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>

#include "base/include/vector.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fiber/testing/compose_draw_recording_contract_test_helpers.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"

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
    SCOPED_TRACE(id);
    DisplayListBuilder sentinel;
    Prefix(&sentinel);

    output_ = sentinel.Build();
    const Bytes output_before(output_);
    shadow_ = false;
    const auto error = detail::DecodeV1DrawDisplayList(wire, pixels, phase,
                                                       &output_, &shadow_);

    if (error) {
      output_before.Expect(output_);
      EXPECT_FALSE(shadow_);
    }
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
    const auto error = detail::DecodeV1DrawDisplayList(
        wire, pixels, ComposeDrawPhase::kBehind, &output, &shadow);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, Code::kMalformed);
    EXPECT_EQ(error->command_index, DrawRecordingError::kNoCommand);
    EXPECT_EQ(output.GetContentItemsSize(), 0u);
    EXPECT_EQ(output.GetContentDataSize(), 0u);
    EXPECT_FALSE(shadow);
  }
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

TEST_F(DrawDisplayListDecoderTest,
       RuntimeTablesAndExpandedDagHaveFixedBudgets) {
  for (const auto& table : {std::pair<unsigned, unsigned>{3, 65536},
                            {4, 65536},
                            {5, 65536},
                            {6, 4096}}) {
    const auto wire =
        Eval("(()=>{let w=['lynx.draw.recording',1,0,[],[],[],[],[],[]];w[" +
             std::to_string(table.first) + "]=Array(" +
             std::to_string(table.second + 1) + ").fill([]);return w})()");
    Reject("table-budget", wire, Code::kCapacity);
  }
  auto dag = Eval(
      "(()=>{let a=[];for(let i=0;i<19;i++)a=[a,a];return "
      "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
  Reject("expanded-dag-budget", dag, Code::kCapacity);
}

TEST_F(DrawDisplayListDecoderTest, DepthAndUtf8PayloadBoundariesAreExact) {
  for (unsigned depth : {64u, 65u}) {
    const auto wire =
        Eval("(()=>{let a=[];for(let i=4;i<" + std::to_string(depth) +
             ";i++)a=[a];return "
             "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
    if (depth == 64) {
      ASSERT_FALSE(Run("depth-64", wire));
      EXPECT_EQ(output_.GetContentItemsSize(), 0u);
    } else {
      Reject("depth-65", wire, Code::kCapacity);
    }
  }
  // Envelope scalars and the metadata key consume 36 bytes of the SDK budget.
  std::string payload(16 * 1024 * 1024 - 38, 'x');
  payload += "α";
  auto wire = Parse("[\"lynx.draw.recording\",1,0,[],[],[],[],[],[]]");
  auto entry = lepus::CArray::Create();
  entry->push_back(Value("k"));
  entry->push_back(Value(payload));
  wire.GetProperty(8u).Array()->push_back(Value(entry));
  ASSERT_FALSE(Run("payload-exact", wire));
  EXPECT_EQ(output_.GetContentItemsSize(), 0u);
  payload += 'x';
  entry->set(1, Value(payload));
  Reject("payload-over", wire, Code::kCapacity);
}

// Temporary coverage for the value-only entry point; schema and command
// validation replace these staging expectations in the following layers.
TEST_F(DrawDisplayListDecoderTest, EmptySuccessAndErrorsAreAtomic) {
  auto wire = Parse("[\"lynx.draw.recording\",1,0,[],[],[],[],[],[]]");
  ASSERT_FALSE(Run("empty", wire));
  EXPECT_EQ(output_.GetContentItemsSize(), 0u);
  EXPECT_EQ(output_.GetContentDataSize(), 0u);
  DisplayListBuilder sentinel;
  Prefix(&sentinel);
  auto output = sentinel.Build();
  const Bytes before(output);
  bool shadow = true;
  const auto error = detail::DecodeV1DrawDisplayList(
      Parse(Wire()), 1, ComposeDrawPhase::kBehind, &output, &shadow);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kUnsupported);
  EXPECT_EQ(error->command_index, DrawRecordingError::kNoCommand);
  EXPECT_TRUE(shadow);
  before.Expect(output);
}

TEST_F(DrawDisplayListDecoderTest, EnvelopeAndGenericTailsAreValidated) {
  Reject("row", Parse("[]"), Code::kMalformed);
  auto wire = Parse(Wire());
  wire.Array()->set(0, Value("other"));
  Reject("discriminator", wire, Code::kVersion);
  wire = Parse(Wire());
  wire.Array()->set(1, Value(2));
  Reject("major", wire, Code::kVersion);
  wire = Parse(Wire());
  wire.Array()->set(2, Value(-1));
  Reject("minor", wire, Code::kMalformed);
  Reject("duplicate-metadata", Parse(Wire("[]", "[]", "[[\"k\",0],[\"k\",1]]")),
         Code::kMalformed);
  for (const char* features : {"null", "[1]", "[[]]", "[{}]"}) {
    auto input = Parse(Wire());
    input.Array()->set(7, Parse(features));
    Reject("malformed-features", input, Code::kMalformed);
  }
  wire = Parse(Wire());
  wire.GetProperty(4u).GetProperty(0u).Array()->set(
      1, Value(std::numeric_limits<double>::infinity()));
  Reject("nonfinite-before-deferred", wire, Code::kMalformed);
}

TEST_F(DrawDisplayListDecoderTest, ExactExpandedNodeBudget) {
  // Root + nine slots + one metadata row + its two slots use 13 nodes.
  for (unsigned nodes : {262131u, 262132u}) {
    const auto wire =
        Eval("(()=>{let a=Array(" + std::to_string(nodes) +
             ").fill(null);return "
             "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
    if (nodes == 262131) {
      ASSERT_FALSE(Run("nodes-exact", wire));
      EXPECT_EQ(output_.GetContentItemsSize(), 0u);
    } else {
      Reject("nodes-over", wire, Code::kCapacity);
    }
  }
}

TEST_F(DrawDisplayListDecoderTest, ExactTablesDeferWithoutAcceptingSemantics) {
  for (const auto& table : {std::pair<unsigned, unsigned>{3, 65536},
                            {4, 65536},
                            {5, 65536},
                            {6, 4096}}) {
    const auto wire =
        Eval("(()=>{let w=['lynx.draw.recording',1,0,[],[],[],[],[],[]];w[" +
             std::to_string(table.first) + "]=Array(" +
             std::to_string(table.second) + ").fill([]);return w})()");
    Reject("table-exact-deferred", wire, Code::kUnsupported);
  }
}

}  // namespace
}  // namespace lynx::tasm::testing
