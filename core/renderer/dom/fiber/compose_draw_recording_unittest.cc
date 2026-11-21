// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// SIZE_OK: Fixed-schema fixtures and hostile input cases belong to this
// decoder.
#include <cstring>
#include <iostream>
#include <limits>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fiber/testing/compose_draw_recording_contract_test_helpers.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "gfx/public/display_list_reader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx::tasm::testing {
namespace {
using Value = lepus::Value;
using Code = DrawRecordingError::Code;
#include "core/renderer/dom/fiber/testing/compose_draw_recording_fixtures.inc"

Value Json(const rapidjson::Value& v) {
  if (v.IsNumber()) return Value(v.GetDouble());
  if (v.IsBool()) return Value(v.GetBool());
  if (v.IsString())
    return Value(std::string(v.GetString(), v.GetStringLength()));
  if (v.IsArray()) {
    auto a = lepus::CArray::Create();
    for (const auto& item : v.GetArray()) a->push_back(Json(item));
    return Value(std::move(a));
  }
  if (v.IsObject()) {
    auto d = lepus::Dictionary::Create();
    for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
      d->SetValue(i->name.GetString(), Json(i->value));
    return Value(std::move(d));
  }
  return {};
}
Value Parse(const char* text) {
  rapidjson::Document d;
  d.Parse(text);
  EXPECT_FALSE(d.HasParseError());
  return Json(d);
}
Value Array(std::initializer_list<Value> items) {
  auto a = lepus::CArray::Create();
  for (const auto& v : items) a->push_back(v);
  return Value(std::move(a));
}
rapidjson::Document Fixtures() {
  rapidjson::Document document;
  for (const auto& f : kComposeDrawRecordingFixtures)
    if (std::strcmp(f.family, "recording-v1") == 0) document.Parse(f.json);
  EXPECT_FALSE(document.HasParseError());
  return document;
}
const rapidjson::Value& Case(const rapidjson::Document& document,
                             const char* name) {
  for (const auto& c : document["cases"].GetArray())
    if (std::strcmp(c["id"].GetString(), name) == 0) return c;
  ADD_FAILURE() << "Missing authored fixture " << name;
  return document["cases"][0];
}
void Change(Value wire, const rapidjson::Value& changes) {
  for (const auto& change : changes.GetArray()) {
    const auto& indices = change[0];
    auto target = wire;
    for (rapidjson::SizeType i = 0; i + 1 < indices.Size(); ++i)
      target = target.GetProperty(indices[i].GetUint());
    ASSERT_TRUE(target.Array()->set(indices[indices.Size() - 1].GetUint(),
                                    Json(change[1])));
  }
}
DisplayList Sentinel() {
  DisplayListBuilder builder;
  contract_fixture::Prefix(&builder);
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
  EXPECT_EQ(output.RootNeedClipBounds(), expected.RootNeedClipBounds());
}
std::optional<DrawRecordingError> Compile(
    const Value& wire, DisplayList* output,
    ComposeDrawPhase phase = ComposeDrawPhase::kBehind) {
  bool shadow = false;
  const auto error =
      detail::DecodeV1DrawDisplayList(wire, 1, phase, output, &shadow);
  if (error) EXPECT_FALSE(shadow);
  return error;
}
void ExpectSolid(const DisplayList& output, float x, uint32_t color) {
  DisplayListReader reader(output);
  ASSERT_TRUE(reader.HasNext());
  const auto& box = reader.Next();
  ASSERT_EQ(box.type, DisplayListOpType::kRecordBox);
  EXPECT_FLOAT_EQ(box.payload.record_box.x, x);
  ASSERT_TRUE(reader.HasNext());
  const auto& fill = reader.Next();
  ASSERT_EQ(fill.type, DisplayListOpType::kFill);
  EXPECT_EQ(fill.payload.fill.color, color);
}
TEST(ComposeDrawRecordingDecoderTest,
     CanonicalCoreAndKnownUnsupportedAreFullyOwned) {
  auto document = Fixtures();
  const auto check = [&](const Value& wire, const rapidjson::Value& expected) {
    auto out = Sentinel();
    const auto error = Compile(wire, &out);
    const bool unsupported =
        std::strcmp(expected["currentPolyfill"].GetString(), "unsupported") ==
        0;
    EXPECT_EQ(static_cast<bool>(error), unsupported);
    if (error) {
      EXPECT_EQ(error->code, Code::kUnsupported);
      ExpectSentinel(out);
    }
  };
  for (const auto& c : document["cases"].GetArray()) {
    SCOPED_TRACE(c["id"].GetString());
    const auto clip = c["expected"].HasMember("currentPolyfill") &&
                      std::strcmp(c["id"].GetString(), "empty-clip") == 0;
    if (clip) {
      DisplayList out;
      ASSERT_FALSE(Compile(Json(c["wire"]), &out, ComposeDrawPhase::kClip));
      DisplayListReader reader(out);
      ASSERT_EQ(reader.Remaining(), 1u);
      EXPECT_EQ(reader.Next().type, DisplayListOpType::kClipRect);
    } else
      check(Json(c["wire"]), c["expected"]);
  }
  for (const auto& variant : document["variants"].GetArray()) {
    SCOPED_TRACE(variant["id"].GetString());
    const auto& base = Case(document, variant["base"].GetString());
    auto wire = Json(base["wire"]);
    Change(wire, variant["changes"]);
    check(wire, variant["expected"].HasMember("currentPolyfill")
                    ? variant["expected"]
                    : base["expected"]);
  }
}

TEST(ComposeDrawRecordingDecoderTest,
     UnversionedArraysRejectWithoutReplacingOwnedRecording) {
  for (const auto& sample :
       {std::pair<const char*, Code>{"[[1,0,0,0,10]]", Code::kMalformed},
        {"[]", Code::kMalformed},
        {"[[1,0,0,10,10,4294967295],[],[],[],[],[],[],[],[]]",
         Code::kVersion}}) {
    SCOPED_TRACE(sample.first);
    auto out = Sentinel();
    const auto error = Compile(Parse(sample.first), &out);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, sample.second);
    EXPECT_EQ(error->command_index, DrawRecordingError::kNoCommand);
    EXPECT_TRUE(error->name_space.empty());
    ExpectSentinel(out);
  }
}

TEST(ComposeDrawRecordingDecoderTest,
     CanonicalInvalidTailsAndUnknownSemanticsAreAtomic) {
  auto document = Fixtures();
  for (const auto& error : document["errors"].GetArray()) {
    SCOPED_TRACE(error["id"].GetString());
    auto input = Json(Case(document, error["base"].GetString())["wire"]);
    Change(input, error["changes"]);
    DisplayList out = Sentinel();
    const auto failure = Compile(input, &out);
    ASSERT_TRUE(failure);
    ExpectSentinel(out);
    if (std::strcmp(error["error"].GetString(), "version") == 0)
      EXPECT_EQ(failure->code, Code::kVersion);
    if (std::strcmp(error["id"].GetString(), "unsupported-tail") == 0) {
      EXPECT_EQ(failure->code, Code::kUnsupported);
      EXPECT_EQ(failure->name_space, "unknown");
    }
  }
}
// Materialize the authored eight-slot recipes. This constructs inputs only;
// depth/outcome expectations come from the frozen fixture, not the decoder.
Value TreeWire(Value base, const char* slot, unsigned depth, Value* cycle) {
  Value payload = Array({Value(0)}), parent;
  uint32_t index;
  unsigned base_depth;
  auto extension =
      Array({Value(255), Value("unassigned"), Value(1), Value(), Array({})});
  if (!std::strcmp(slot, "command-extension") ||
      !std::strcmp(slot, "path-extension")) {
    base.GetProperty(!std::strcmp(slot, "command-extension") ? 3u : 4u)
        .Array()
        ->push_back(extension);
    parent = extension;
    index = 3;
    base_depth = 4;
  } else if (!std::strcmp(slot, "source-extension")) {
    base.GetProperty(5u).GetProperty(0u).Array()->set(0, extension);
    parent = extension;
    index = 3;
    base_depth = 5;
  } else if (!std::strcmp(slot, "resource")) {
    auto resource = Array({Value("unassigned"), Value(1), Value()});
    base.GetProperty(6u).Array()->push_back(resource);
    parent = resource;
    index = 2;
    base_depth = 4;
  } else if (!std::strcmp(slot, "effect")) {
    base.GetProperty(5u).GetProperty(0u).GetProperty(3u).Array()->push_back(
        extension);
    parent = extension;
    index = 3;
    base_depth = 6;
  } else if (!std::strcmp(slot, "path-effect")) {
    base.GetProperty(5u).GetProperty(0u).Array()->set(
        2, Array({Value(2), Value(1), Value(0), Value(0), Value(4), Array({}),
                  Value(0), extension}));
    parent = extension;
    index = 3;
    base_depth = 6;
  } else if (!std::strcmp(slot, "filter-extension")) {
    base.GetProperty(5u).GetProperty(0u).GetProperty(1u).Array()->set(
        3, extension);
    parent = extension;
    index = 3;
    base_depth = 6;
  } else {
    EXPECT_STREQ(slot, "metadata");
    auto entry = Array({Value("diagnostic"), Value()});
    base.GetProperty(8u).Array()->push_back(entry);
    parent = entry;
    index = 1;
    base_depth = 4;
  }
  if (cycle) {
    payload.Array()->set(0, payload);
    *cycle = payload;
  } else
    for (unsigned i = base_depth; i < depth; ++i) payload = Array({payload});
  parent.Array()->set(index, payload);
  return base;
}
TEST(ComposeDrawRecordingDecoderTest,
     WholeEnvelopeDepthAndCyclesPrecedeUnknownSemantics) {
  auto document = Fixtures();
  for (const auto& slot : document["valueTrees"]["slots"].GetArray()) {
    for (const auto& c : document["valueTrees"]["cases"].GetArray()) {
      SCOPED_TRACE(slot.GetString());
      SCOPED_TRACE(c["id"].GetString());
      Value cycle;
      auto input =
          TreeWire(Json(Case(document, "rect")["wire"]), slot.GetString(),
                   c.HasMember("depth") ? c["depth"].GetUint() : 4,
                   c.HasMember("cycle") ? &cycle : nullptr);
      DisplayList out = Sentinel();
      const auto error = Compile(input, &out);
      // Always break the temporary native cycle, even if assertions fail.
      if (c.HasMember("cycle")) cycle.Array()->set(0, Value());
      const char* expected =
          c[!std::strcmp(slot.GetString(), "metadata") ? "metadataSemantic"
                                                       : "semantic"]
              .GetString();
      if (!std::strcmp(expected, "ok")) {
        ASSERT_FALSE(error);
        EXPECT_EQ(out.GetContentItemsSize(), 2u);
        ExpectSolid(out, 2, 0x40112233u);
      } else {
        ASSERT_TRUE(error);
        ExpectSentinel(out);
        EXPECT_EQ(error->code, !std::strcmp(expected, "depth") ? Code::kCapacity
                               : !std::strcmp(expected, "unsupported")
                                   ? Code::kUnsupported
                                   : Code::kMalformed);
      }
      std::cout << "VALUE_TREE slot=" << slot.GetString()
                << " case=" << c["id"].GetString()
                << " observed=" << (error ? static_cast<int>(error->code) : 0)
                << std::endl;
    }
  }
}
TEST(ComposeDrawRecordingDecoderTest,
     PhysicalDoubleMixedRuntimeOwnershipSurvivesInputDestruction) {
  auto document = Fixtures();
  DisplayList out, clip;
  {
    auto wire = Parse(
        "[\"lynx.draw.recording\",1,0,[[1,0,0],[1,0,1],[9,0,0,-2,3,4,-1,0,1]],"
        "[[1,16777217.25,-2.5,40,20]],"
        "[[[1,2148606515],[0.5,0,true,null],[1],[]],"
        "[[2,40,0,-10,0,[2148606515,4294967295,4278190335],[0,0.5,0.5],0,[1,0,"
        "0,1,0,0]],"
        "[0.5,0,true,null],[1],[]]],[],"
        "[\"drawing.shadow\",\"paint.blend.SrcOver\",\"paint.linear\",\"paint."
        "solid\",\"paint.srgb\",\"paint.tile.Clamp\",\"path.rect\"],"
        "[[\"diagnostic\",[\"owned\",true,[7.25]]]]]");
    ASSERT_FALSE(Compile(wire, &out));
    wire.GetProperty(4u).GetProperty(0u).Array()->set(1, Value(99));
    wire.GetProperty(5u)
        .GetProperty(1u)
        .GetProperty(0u)
        .GetProperty(5u)
        .Array()
        ->set(0, Value(0));
    wire.GetProperty(5u)
        .GetProperty(1u)
        .GetProperty(0u)
        .GetProperty(6u)
        .Array()
        ->set(1, Value(0));
    wire.GetProperty(8u).GetProperty(0u).GetProperty(1u).Array()->set(
        0, Value("changed"));
    ASSERT_FALSE(Compile(Json(Case(document, "empty-clip")["wire"]), &clip,
                         ComposeDrawPhase::kClip));
  }
  DisplayListReader reader(out);
  unsigned fills = 0, gradients = 0, shadows = 0;
  while (reader.HasNext()) {
    const auto& item = reader.Next();
    if (item.type == DisplayListOpType::kRecordBox)
      EXPECT_TRUE(std::isfinite(item.payload.record_box.x));
    if (item.type == DisplayListOpType::kFill) {
      ++fills;
      EXPECT_EQ(item.payload.fill.color, 0x40112233u);
    }
    if (item.type == DisplayListOpType::kLinearGradient) {
      ++gradients;
      EXPECT_EQ(reader.Colors(item)[0], 0x40112233u);
      EXPECT_FLOAT_EQ(item.payload.linear_gradient.angle, 270);
    }
    if (item.type == DisplayListOpType::kBoxShadow) {
      ++shadows;
      EXPECT_FLOAT_EQ(item.payload.box_shadow.blur_radius, 4);
    }
  }
  EXPECT_EQ(fills, 1u);
  EXPECT_EQ(gradients, 1u);
  EXPECT_EQ(shadows, 1u);
  DisplayListReader clip_reader(clip);
  ASSERT_EQ(clip_reader.Remaining(), 1u);
  const auto& item = clip_reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kClipRect);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.w, 0);
}

TEST(ComposeDrawRecordingDecoderTest,
     NonfiniteAllMatricesIdsAndSemanticArraysReject) {
  auto document = Fixtures();
  for (double n : {std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::quiet_NaN()}) {
    for (int which = 0; which < 7; ++which) {
      SCOPED_TRACE(which);
      auto wire = Json(Case(document, which == 1   ? "linear-hard-stops"
                                      : which == 2 ? "matrix-order"
                                                   : "rect")["wire"]);
      switch (which) {
        case 0:
          wire.GetProperty(4u).GetProperty(0u).Array()->set(1, Value(n));
          break;
        case 1:
          wire.GetProperty(5u)
              .GetProperty(0u)
              .GetProperty(0u)
              .GetProperty(8u)
              .Array()
              ->set(4, Value(n));
          break;
        case 2:
          wire.GetProperty(3u).GetProperty(1u).Array()->set(1, Value(n));
          break;
        case 3:
          wire.GetProperty(5u).GetProperty(0u).GetProperty(1u).Array()->set(
              0, Value(n));
          break;
        case 4:
          wire.GetProperty(8u).Array()->push_back(
              Array({Value("diagnostic"), Array({Value(n)})}));
          break;
        case 5:
          wire.GetProperty(3u).GetProperty(0u).Array()->set(1, Value(n));
          break;
        case 6:
          wire.GetProperty(5u).GetProperty(0u).GetProperty(1u).Array()->set(
              3, Array({Value(2),
                        Array({Value(1), Value(0), Value(0), Value(0),
                               Value(n), Value(0), Value(1), Value(0),
                               Value(0), Value(0), Value(0), Value(0),
                               Value(1), Value(0), Value(0), Value(0),
                               Value(0), Value(0), Value(1), Value(0)})}));
          break;
      }
      auto out = Sentinel();
      ASSERT_TRUE(Compile(wire, &out));
      ExpectSentinel(out);
    }
  }
  auto wire = Json(Case(document, "balanced-state-higher-minor")["wire"]);
  wire.GetProperty(3u).Array()->set(
      1, Array({Value(4), Value(1e308), Value(0), Value(0), Value(1), Value(0),
                Value(0)}));
  wire.GetProperty(3u).Array()->set(
      2, Array({Value(4), Value(2), Value(0), Value(0), Value(1), Value(0),
                Value(0)}));
  wire.GetProperty(3u).Array()->push_back(Array({Value(3)}));
  auto out = Sentinel();
  const auto error = Compile(wire, &out);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kOverflow);
  ExpectSentinel(out);
}
TEST(ComposeDrawRecordingDecoderTest,
     RuntimeArraysAreBoundedWithoutDeepConversion) {
  auto runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  runtime->Initialize();
  const auto eval = [&](const std::string& script) {
    Value value;
    EXPECT_TRUE(
        runtime::MTSRuntime::ToQuickContext(runtime.get())
            ->EvalBuf(script.data(), script.size(), value, "decode.js"));
    return value;
  };
  // A real runtime-produced envelope, not preconverted CArrays.
  auto source = eval(
      "globalThis.input = "
      "[\"lynx.draw.recording\",1,0,[[1,0,0]],[[1,16777217.25,0,10,10]],"
      "[[[1,2148606515],[0.5,0,true,null],[1],[]]],[],"
      "[\"paint.blend.SrcOver\",\"paint.solid\",\"paint.srgb\",\"path.rect\"],["
      "[\"diagnostic\",[\"owned\"]]]]; input");
  ASSERT_TRUE(source.IsJSValue());
  DisplayList out;
  ASSERT_FALSE(Compile(source, &out));
  eval("input[4][0][1]=99; input[8][0][1][0]='changed'; input=null");
  source = {};
  ExpectSolid(out, 16777217.25, 0x40112233u);
  auto cyclic = eval(
      "(function(){let a=[]; a.push(a); return "
      "[\"lynx.draw.recording\",1,0,[],[],[],[],[],[[\"x\",a]]];})()");
  out = Sentinel();
  const auto error = Compile(cyclic, &out);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kMalformed);
  ExpectSentinel(out);
}
TEST(ComposeDrawRecordingDecoderTest,
     PathDepth64And65AndExactUtf8PayloadBudget) {
  auto wire = Parse(
      "[\"lynx.draw.recording\",1,0,[],[],[],[],[\"path.border-clip\",\"path."
      "rect\"],[]]");
  wire.GetProperty(4u).Array()->push_back(
      Array({Value(1), Value(0), Value(0), Value(10), Value(10)}));
  for (int i = 1; i < 64; ++i)
    wire.GetProperty(4u).Array()->push_back(
        Array({Value(3), Value(i - 1), Value(-1)}));
  DisplayList out;
  const auto supported_depth = Compile(wire, &out);
  ASSERT_TRUE(supported_depth);
  EXPECT_EQ(supported_depth->code, Code::kUnsupported);
  wire.GetProperty(4u).Array()->push_back(
      Array({Value(3), Value(63), Value(-1)}));
  out = Sentinel();
  auto error = Compile(wire, &out);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kCapacity);
  ExpectSentinel(out);
  // Envelope scalars and the metadata key consume 36 bytes of the SDK budget.
  std::string payload(16 * 1024 * 1024 - 38, 'x');
  payload += "α";
  wire = Parse("[\"lynx.draw.recording\",1,0,[],[],[],[],[],[]]");
  auto metadata = wire.GetProperty(8u).Array();
  metadata->push_back(Array({Value("k"), Value(payload)}));
  ASSERT_FALSE(Compile(wire, &out));
  EXPECT_EQ(out.GetContentItemsSize(), 0u);
  payload += 'x';
  metadata->set(0, Array({Value("k"), Value(payload)}));
  out = Sentinel();
  error = Compile(wire, &out);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kCapacity);
  ExpectSentinel(out);
}
TEST(ComposeDrawRecordingDecoderTest,
     SaveAndLayerRestoreMatrixBeforeFurtherValidation) {
  for (bool layer : {false, true}) {
    SCOPED_TRACE(layer);
    auto document = Fixtures();
    auto wire = Json(Case(document, "rect")["wire"]);
    auto commands = Array(
        {layer
             ? Array({Value(6), Array({Value(0), Value(0), Value(1), Value(1)}),
                      Value(0)})
             : Array({Value(2)}),
         Array({Value(4), Value(1e308), Value(0), Value(0), Value(1), Value(0),
                Value(0)}),
         Array({Value(layer ? 7 : 3)}),
         Array({Value(4), Value(2), Value(0), Value(0), Value(1), Value(0),
                Value(0)})});
    wire.Array()->set(3, commands);
    const auto features = wire.GetProperty(7u);
    features.Array()->Insert(0, Value("drawing.state"));
    if (layer) features.Array()->Insert(0, Value("drawing.layer"));
    DisplayList out;
    const auto error = Compile(wire, &out);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, Code::kUnsupported);
    // Restored matrix is finite; schema validation completes before the
    // backend rejects nonidentity state or a layer.
  }
  for (unsigned depth : {64u, 65u}) {
    auto wire = Parse(
        "[\"lynx.draw.recording\",1,0,[],[],[],[],[\"drawing.state\"],[]]");
    auto commands = wire.GetProperty(3u);
    for (unsigned i = 0; i < depth; ++i)
      commands.Array()->push_back(Array({Value(2)}));
    for (unsigned i = 0; i < depth; ++i)
      commands.Array()->push_back(Array({Value(3)}));
    auto out = Sentinel();
    const auto error = Compile(wire, &out);
    if (depth == 64) {
      ASSERT_FALSE(error);
      EXPECT_EQ(out.GetContentItemsSize(), 0u);
    } else {
      ASSERT_TRUE(error);
      EXPECT_EQ(error->code, Code::kCapacity);
      ExpectSentinel(out);
    }
  }
}
TEST(ComposeDrawRecordingDecoderTest,
     EmptyArrayDagHasAggregateStructuralBound) {
  Value payload = Array({});
  for (int d = 0; d < 19; ++d) payload = Array({payload, payload});
  auto wire = Parse("[\"lynx.draw.recording\",1,0,[],[],[],[],[],[]]");
  wire.GetProperty(8u).Array()->push_back(Array({Value("k"), payload}));
  auto out = Sentinel();
  const auto error = Compile(wire, &out);
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kCapacity);
  ExpectSentinel(out);
}

TEST(ComposeDrawRecordingDecoderTest,
     LargeEmptyArrayHasAggregateStructuralBound) {
  auto payload = Array({});
  const auto empty = Array({});
  for (unsigned i = 0; i < 300000; ++i) payload.Array()->push_back(empty);
  auto wire = Parse("[\"lynx.draw.recording\",1,0,[],[],[],[],[],[]]");
  wire.GetProperty(8u).Array()->push_back(Array({Value("k"), payload}));
  auto out = Sentinel();
  const auto error = Compile(wire, &out);
  std::cout << "STRUCTURAL_EMPTY_ARRAY children=300000 scalarBytes=36 "
            << "capacityError=" << static_cast<bool>(error) << std::endl;
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, Code::kCapacity);
  ExpectSentinel(out);
}

TEST(ComposeDrawRecordingDecoderTest, AdvisoryFeaturesPreserveOwnedSemantics) {
  auto document = Fixtures();
  for (const auto* declarations :
       {"[]", "[\"unknown.future\"]",
        "[\"path.rect\",\"paint.solid\",\"path.rect\"]"}) {
    auto wire = Json(Case(document, "rect")["wire"]);
    const auto features = Parse(declarations);
    wire.Array()->set(7, features);
    DisplayList out;
    ASSERT_FALSE(Compile(wire, &out));
    ASSERT_EQ(out.GetContentItemsSize(), 2u);
    ExpectSolid(out, 2, 0x40112233u);
  }
  for (const auto* declarations : {"null", "[1]", "[[]]", "[{}]"}) {
    auto wire = Json(Case(document, "rect")["wire"]);
    wire.Array()->set(7, Parse(declarations));
    auto out = Sentinel();
    ASSERT_TRUE(Compile(wire, &out));
    ExpectSentinel(out);
  }
}

TEST(ComposeDrawRecordingDecoderTest, UnusedTypedRowsStillValidateAtomically) {
  auto document = Fixtures();
  for (unsigned table : {4u, 5u}) {
    auto wire = Json(Case(document, "rect")["wire"]);
    wire.GetProperty(table).Array()->push_back(
        table == 4 ? Array({Value(1), Value(0), Value(0), Value(-1), Value(1)})
                   : Parse("[[1,4294967296],[1,0,true,null],[1],[]]"));
    auto out = Sentinel();
    ASSERT_TRUE(Compile(wire, &out));
    ExpectSentinel(out);
  }
}

}  // namespace
}  // namespace lynx::tasm::testing
