// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <iostream>

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
  std::vector<uint8_t> items, data, properties;
  std::vector<const PaintImage*> images;
  std::vector<const gfx::Path*> paths;
  std::vector<int> layers;
  float dx, dy;
  bool root_clip;
  explicit Bytes(const DisplayList& list)
      : dx(list.GetRenderOffset()[0]),
        dy(list.GetRenderOffset()[1]),
        root_clip(list.RootNeedClipBounds()) {
    if (list.GetContentItemsByteSize())
      items.assign(list.GetContentItemsData(),
                   list.GetContentItemsData() + list.GetContentItemsByteSize());
    if (list.GetContentDataSize())
      data.assign(list.GetContentData(),
                  list.GetContentData() + list.GetContentDataSize());
    if (list.GetSubtreePropertiesSize()) {
      const auto* start =
          reinterpret_cast<const uint8_t*>(list.GetSubtreePropertiesData());
      properties.assign(start, start + list.GetSubtreePropertiesSize() *
                                           sizeof(SubtreeProperty));
    }
    auto& resource_access = const_cast<DisplayList&>(list);
    for (const auto& image : resource_access.Images())
      images.push_back(image.get());
    layers.assign(resource_access.SubLayers().begin(),
                  resource_access.SubLayers().end());
    for (size_t i = 0; i < list.GetPathCount(); ++i)
      paths.push_back(list.GetPath(i));
  }
  void Expect(const DisplayList& list) const {
    const Bytes after(list);
    EXPECT_EQ(after.items, items);
    EXPECT_EQ(after.data, data);
    EXPECT_EQ(after.properties, properties);
    EXPECT_EQ(after.images, images);
    EXPECT_EQ(after.paths, paths);
    EXPECT_EQ(after.layers, layers);
    EXPECT_EQ(after.dx, dx);
    EXPECT_EQ(after.dy, dy);
    EXPECT_EQ(after.root_clip, root_clip);
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
      const detail::DecodeLimits& limits = {},
      ComposeDrawPhase phase = ComposeDrawPhase::kBehind,
      bool initial_shadow = false) {
    DisplayListBuilder destination, sentinel;
    Prefix(&destination);
    Prefix(&sentinel);
    const Bytes before(destination.CurrentDisplayList());
    output_ = sentinel.Build();
    const Bytes output_before(output_);
    shadow_ = initial_shadow;
    const auto error = detail::DecodeDrawDisplayList(
        wire, pixels, phase, limits, destination, &output_, &shadow_);
    before.Expect(destination.CurrentDisplayList());
    EXPECT_EQ(destination.BoxModelCount(), 1u);
    if (error) {
      output_before.Expect(output_);
      EXPECT_EQ(shadow_, initial_shadow);
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
              << "\",\"shadow\":" << shadow_ << ",\"prefix_unchanged\":true"
              << ",\"output_items\":" << output_.GetContentItemsSize()
              << ",\"output_data\":" << output_.GetContentDataSize() << "}"
              << std::endl;
    return error;
  }
  void Reject(const char* id, const Value& wire, Code code,
              size_t command = DrawRecordingError::kNoCommand,
              const char* name_space = "",
              const detail::DecodeLimits& limits = {},
              ComposeDrawPhase phase = ComposeDrawPhase::kBehind) {
    for (double pixels : {1., 2., 3.})
      for (bool flag : {false, true}) {
        SCOPED_TRACE(id);
        const auto error = Run(id, wire, pixels, limits, phase, flag);
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

TEST_F(DrawDisplayListDecoderTest, CompilesFinalStorageWithoutTouchingPrefix) {
  DisplayListBuilder destination;
  Prefix(&destination);
  const auto prefix_items =
      destination.CurrentDisplayList().GetContentItemsSize();
  DisplayList output;
  bool shadow = true;
  ASSERT_FALSE(detail::DecodeDrawDisplayList(Parse(Wire()), 2,
                                             ComposeDrawPhase::kBehind, {},
                                             destination, &output, &shadow));
  EXPECT_FALSE(shadow);
  EXPECT_EQ(destination.CurrentDisplayList().GetContentItemsSize(),
            prefix_items);
  EXPECT_EQ(destination.BoxModelCount(), 1u);
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
TEST_F(DrawDisplayListDecoderTest, RichOwnerResourcesAndLateFailureAreAtomic) {
  DisplayListBuilder destination(17, 19), sentinel(23, 29);
  const auto image = fml::MakeRefCounted<PaintImage>(42);
  const auto path = gfx::Path::MakeRect(10, 20);
  for (auto* builder : {&destination, &sentinel}) {
    Prefix(builder);
    builder->Opacity(.5).ClipPathProperty(path).DrawImage(image, 0);
    builder->DrawView(101, 3, 5);
  }
  const Bytes before(destination.CurrentDisplayList());
  DisplayList output = sentinel.Build();
  const Bytes output_before(output);
  for (bool flag : {false, true}) {
    bool shadow = flag;
    auto error = detail::DecodeDrawDisplayList(
        Parse(Wire("[[9,0,0,1,2,3,4,0,1],[1,1,0]]",
                   "[[1,0,0,40,20],[1,1e40,0,40,20]]")),
        1, ComposeDrawPhase::kBehind, {}, destination, &output, &shadow);
    ASSERT_TRUE(error);
    EXPECT_EQ(error->code, Code::kOverflow);
    EXPECT_EQ(error->command_index, 1u);
    before.Expect(destination.CurrentDisplayList());
    output_before.Expect(output);
    EXPECT_EQ(destination.BoxModelCount(), 1u);
    EXPECT_EQ(shadow, flag);
  }
  bool shadow = true;
  ASSERT_FALSE(detail::DecodeDrawDisplayList(Parse(Wire()), 1,
                                             ComposeDrawPhase::kBehind, {},
                                             destination, &output, &shadow));
  before.Expect(destination.CurrentDisplayList());
  EXPECT_EQ(output.GetSubtreePropertiesSize(), 0u);
  EXPECT_EQ(output.GetPathCount(), 0u);
  EXPECT_TRUE(output.Images().empty());
  EXPECT_TRUE(output.SubLayers().empty());
  EXPECT_FALSE(output.RootNeedClipBounds());
  EXPECT_FLOAT_EQ(output.GetRenderOffset()[0], 0);
  EXPECT_FLOAT_EQ(output.GetRenderOffset()[1], 0);
  EXPECT_FALSE(shadow);
  std::cout
      << "RICH_OWNER items_data_properties_resources_offsets_flags=unchanged "
         "standalone_origin=0"
      << std::endl;
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
  detail::DecodeLimits limits;
  limits.max_gradient_stops = 0;
  limits.max_items = 6;
  limits.max_boxes = 3;
  limits.max_data_bytes = 16;
  ASSERT_FALSE(Run("solid-border-exact-budget", wire, 1, limits));
  EXPECT_EQ(output_.GetContentItemsSize(), 3u);
  EXPECT_EQ(output_.GetContentDataSize(), 0u);
  limits.max_items = 5;
  Reject("solid-border-item-budget", wire, Code::kCapacity, 0, "", limits);
  limits.max_items = 6;
  limits.max_boxes = 2;
  Reject("solid-border-box-budget", wire, Code::kCapacity, 0, "", limits);
}

TEST_F(DrawDisplayListDecoderTest, ExactAuthoredRejectionDiagnostics) {
  for (const auto& scene : kUnsupportedScenes)
    Reject(scene.name, Parse(scene.wire), Code::kUnsupported,
           scene.command_index, scene.name_space);
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
         DrawRecordingError::kNoCommand, "", {}, ComposeDrawPhase::kClip);
  wire.GetProperty(4u).Array()->set(0, Parse("[1,0,0,40,20]"));
  Reject("paint-before-phase", wire, Code::kUnsupported,
         DrawRecordingError::kNoCommand, "paint.radial", {},
         ComposeDrawPhase::kClip);
  wire.GetProperty(5u).Array()->set(1, Parse("[[1,1],[1,0,true,null],[1],[]]"));
  Reject("phase-before-command", wire, Code::kUnsupported,
         DrawRecordingError::kNoCommand, "", {}, ComposeDrawPhase::kClip);
  detail::DecodeLimits limits;
  limits.max_items = 2;
  Reject("prefix-before-capability", Parse(Wire("[[1,0,0]]", ellipse)),
         Code::kCapacity, DrawRecordingError::kNoCommand, "", limits);
  Reject("metadata-before-prefix",
         Parse(Wire("[[1,0,0]]", ellipse, "[[42,0]]")), Code::kMalformed,
         DrawRecordingError::kNoCommand, "", limits);
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

TEST_F(DrawDisplayListDecoderTest, PrefixBudgetsChargeOriginalCommand) {
  auto wire = Parse(Wire("[[1,0,0],[1,0,0]]"));
  wire.GetProperty(5u).Array()->set(
      0, Parse("[[2,32,-8,-8,32,[2164195328,4278255360,4278190335,1073742079],"
               "[0,0.5,0.5,1],0,[1,0,0,1,0,0]],[0.5,0,true,null],[1],[]]"));
  for (int budget = 0; budget < 4; ++budget) {
    detail::DecodeLimits limits;
    if (budget == 0) limits.max_items = 8;
    if (budget == 1) limits.max_boxes = 4;
    if (budget == 2) limits.max_data_bytes = 111;
    if (budget == 3) limits.max_gradient_stops = 5;
    Reject("prefix-budget", wire, Code::kCapacity, budget == 3 ? 0 : 1, "",
           limits);
  }
  detail::DecodeLimits exact;
  exact.max_items = 9;
  exact.max_boxes = 5;
  exact.max_data_bytes = 112;
  exact.max_gradient_stops = 6;
  ASSERT_FALSE(Run("exact-prefix-budget", wire, 1, exact));
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
  ASSERT_FALSE(Run("empty-clip", clip, 1, {}, ComposeDrawPhase::kClip));
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

TEST_F(DrawDisplayListDecoderTest, CyclesDagAndExactAggregateBudgets) {
  auto cycle = Eval(
      "(()=>{let a=[];a.push(a);return "
      "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
  Reject("payload-cycle", cycle, Code::kMalformed);
  auto dag = Eval(
      "(()=>{let a=[];for(let i=0;i<3;i++)a=[a,a];return "
      "['lynx.draw.recording',1,0,[],[],[],[],[],[['k',a]]]})()");
  detail::DecodeLimits limits;
  limits.max_payload_bytes = 36;
  limits.max_snapshot_nodes = 27;
  limits.max_owned_value_nodes = 15;
  ASSERT_FALSE(Run("dag-exact-budget", dag, 1, limits));
  --limits.max_snapshot_nodes;
  Reject("dag-snapshot-budget", dag, Code::kCapacity,
         DrawRecordingError::kNoCommand, "", limits);
  ++limits.max_snapshot_nodes;
  --limits.max_owned_value_nodes;
  Reject("dag-value-budget", dag, Code::kCapacity,
         DrawRecordingError::kNoCommand, "", limits);
  auto bytes = Parse(
      "[\"lynx.draw.recording\",1,0,[],[],[],[],[],[[\"k\",\"\u03b1\"]]]");
  limits = {};
  limits.max_payload_bytes = 38;
  ASSERT_FALSE(Run("utf8-exact-budget", bytes, 1, limits));
  --limits.max_payload_bytes;
  Reject("utf8-byte-budget", bytes, Code::kCapacity,
         DrawRecordingError::kNoCommand, "", limits);
}

TEST_F(DrawDisplayListDecoderTest, ObservesRuntimeRowsOnceAndOwnsFinalOutput) {
  auto wire = Eval(
      "(()=>{globalThis.reads=0;globalThis.repeats=0;"
      "function wrap(v){if(!Array.isArray(v))return v;let "
      "a=[];v.forEach((x,i)=>{"
      "let "
      "child=wrap(x),seen=false;Object.defineProperty(a,String(i),{get(){reads+"
      "+;"
      "if(seen){repeats++;return {}}seen=true;return child}})});return a}"
      "return wrap(" +
      Wire() + ")})()");
  ASSERT_FALSE(Run("once-only-row-observation", wire));
  EXPECT_GT(Eval("reads").Number(), 20);
  EXPECT_EQ(Eval("repeats").Number(), 0);
  const Bytes retained(output_);
  wire = {};
  runtime_.reset();
  retained.Expect(output_);
  auto payload = Eval(
      "(()=>{globalThis.payloadReads=0;let row=['k'];"
      "Object.defineProperty(row,'1',{get(){payloadReads++;return "
      "payloadReads===1?"
      "['text',true,7.25]:{}}});return "
      "['lynx.draw.recording',1,0,[],[],[],[],[],[row]]})()");
  ASSERT_FALSE(Run("once-only-payload", payload));
  EXPECT_EQ(Eval("payloadReads").Number(), 1);
}

TEST_F(DrawDisplayListDecoderTest, LimitsPrecedeRuntimeChildGetters) {
  for (unsigned slot = 3; slot <= 6; ++slot) {
    auto wire = Eval(
        "(()=>{globalThis.rowReads=0;let rows=[];for(let i=0;i<8;i++)"
        "Object.defineProperty(rows,String(i),{get(){rowReads++;return []}});"
        "let w=['lynx.draw.recording',1,0,[],[],[],[],[],[]];w[" +
        std::to_string(slot) + "]=rows;return w})()");
    detail::DecodeLimits limits;
    limits.max_commands = limits.max_paths = limits.max_paints =
        limits.max_resources = 0;
    Reject("table-before-getter", wire, Code::kCapacity,
           DrawRecordingError::kNoCommand, "", limits);
    EXPECT_EQ(Eval("rowReads").Number(), 0);
  }
  auto wire = Eval(
      "(()=>{globalThis.childReads=0;let p=[];for(let i=0;i<8;i++)"
      "Object.defineProperty(p,String(i),{get(){childReads++;return []}});"
      "return ['lynx.draw.recording',1,0,[],[],[],[],[],[['k',p]]]})()");
  detail::DecodeLimits limits;
  limits.max_snapshot_nodes = 20;
  Reject("snapshot-before-getter", wire, Code::kCapacity,
         DrawRecordingError::kNoCommand, "", limits);
  EXPECT_EQ(Eval("childReads").Number(), 0);
  limits.max_snapshot_nodes = 21;
  limits.max_owned_value_nodes = 8;
  Reject("value-before-getter", wire, Code::kCapacity,
         DrawRecordingError::kNoCommand, "", limits);
  EXPECT_EQ(Eval("childReads").Number(), 0);
  limits.max_owned_value_nodes = 9;
  ASSERT_FALSE(Run("exact-getter-budget", wire, 1, limits));
  EXPECT_EQ(Eval("childReads").Number(), 8);
  auto deep = Eval(
      "(()=>{globalThis.childReads=0;let p=[];Object.defineProperty(p,'0',"
      "{get(){childReads++;return 0}});for(let d=4;d<65;d++)p=[p];"
      "return ['lynx.draw.recording',1,0,[],[],[],[],[],[['k',p]]]})()");
  Reject("depth-before-getter", deep, Code::kCapacity);
  EXPECT_EQ(Eval("childReads").Number(), 0);
}
}  // namespace
}  // namespace lynx::tasm::testing
