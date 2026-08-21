// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_TELEMETRY_STYLING_CSS_STYLING_BENCHMARK_SUPPORT_H_
#define TESTING_TELEMETRY_STYLING_CSS_STYLING_BENCHMARK_SUPPORT_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"

namespace lynx {
namespace tasm {
namespace benchmark_support {

constexpr int32_t kBenchmarkWidth = 1080;
constexpr int32_t kBenchmarkHeight = 1920;
constexpr float kBenchmarkLayoutsUnitPerPx = 1.f;
constexpr double kBenchmarkPhysicalPixelsPerLayoutUnit = 1.f;
constexpr int32_t kBenchmarkCSSId = 1;

class NoOpPaintingContext : public PaintingCtxPlatformImpl {
 public:
  NoOpPaintingContext() {
    platform_ref_ = std::make_shared<PaintingCtxPlatformRef>();
  }

  void CreatePaintingNode(int id, const std::string& tag,
                          const fml::RefPtr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override {}
  void InsertPaintingNode(int parent, int child, int index) override {}
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override {}
  void DestroyPaintingNode(int parent, int child, int index) override {}
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override {}
  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override {
    return nullptr;
  }
  void StopExposure(const pub::Value& options) override {}
  void ResumeExposure() override {}
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height, uint32_t node_index,
                    bool display_none) override {}
  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override {}
  void Flush() override {}
  void HandleValidate(int tag) override {}
  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}
  std::vector<float> getBoundingClientOrigin(int id) override { return {}; }
  std::vector<float> getWindowSize(int id) override { return {}; }
  std::vector<float> GetRectToWindow(int id) override { return {}; }
  std::vector<float> GetRectToLynxView(int64_t id) override { return {}; }
  std::vector<float> ScrollBy(int64_t id, float width, float height) override {
    return {};
  }
  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback) override {}
  void EnqueueInvoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>& callback)
      override {}
  int32_t GetTagInfo(const std::string& tag_name) override { return 0; }
  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override {
    return false;
  }
  bool NeedAnimationProps() override { return false; }
};

class NoOpDelegate : public ElementManager::Delegate {
 public:
  std::unordered_map<int32_t, LayoutInfoArray> GetSubTreeLayoutInfo(
      int32_t root_id, Viewport viewport) override {
    return {};
  }
  void CreateLayoutNode(int32_t id, const base::String& tag) override {}
  void UpdateLayoutNodeFontSize(int32_t id, double cur_node_font_size,
                                double root_node_font_size,
                                double font_scale) override {}
  void InsertLayoutNode(int32_t parent_id, int32_t child_id,
                        int index) override {}
  void SendAnimationEvent(const std::string& type, int tag,
                          const lepus::Value& dict) override {}
  void RemoveLayoutNodeAtIndex(int32_t parent_id, int index) {}
  void SendNativeCustomEvent(const std::string& name, int tag,
                             const lepus::Value& param_value,
                             const std::string& param_name) override {}
  void InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                              int32_t ref_id) override {}
  void RemoveLayoutNode(int32_t parent_id, int32_t child_id) override {}
  void DestroyLayoutNode(int32_t id) override {}
  void UpdateLayoutNodeStyle(int32_t id, CSSPropertyID css_id,
                             const CSSValue& value) override {}
  void ResetLayoutNodeStyle(int32_t id, CSSPropertyID css_id) override {}
  void UpdateLayoutNodeAttribute(int32_t id, starlight::LayoutAttribute key,
                                 const lepus::Value& value) override {}
  void SetFontFaces(const CSSFontFaceRuleMap& fontfaces) override {}
  void UpdateLayoutNodeByBundle(int32_t id,
                                std::unique_ptr<LayoutBundle> bundle) override {
  }
  void UpdateLayoutNodeProps(int32_t id,
                             const fml::RefPtr<PropBundle>& props) override {}
  void MarkLayoutDirty(int32_t id) override {}
  void AttachLayoutNodeType(int32_t id, const base::String& tag,
                            bool allow_inline,
                            const fml::RefPtr<PropBundle>& props) override {}
  void UpdateLynxEnvForLayoutThread(LynxEnvConfig env) override {}
  void OnUpdateViewport(float width, int width_mode, float height,
                        int height_mode, bool need_layout) override {}
  void SetRootOnLayout(int32_t id) override {}
  void OnUpdateDataWithoutChange() override {}
  void SetPageConfigForLayoutThread(
      const std::shared_ptr<PageConfig>& config) override {}
  void OnErrorOccurred(base::LynxError error) override {}
  void BindPipelineIDWithTimingFlag(
      const PipelineID& pipeline_id,
      const timing::TimingFlag& timing_flag) override {}
  void ReportElementMemoryInfo(int64_t mem_size_bytes,
                               int element_count) override {}
};

class NoOpElementManagerDelegate : public ElementManagerDelegate {
 public:
  void LoadFrameBundle(const std::string& src, FrameElement* element) override {
  }
  void DidFrameBundleLoaded(
      const LazyBundleLoader::CallBackInfo& callback_info) override {}
  void OnFrameRemoved(FrameElement* element) override {}

  PipelineContext* GetCurrentPipelineContext() override { return nullptr; }

  PipelineContext* CreateAndUpdateCurrentPipelineContext(
      const std::shared_ptr<PipelineOptions>& pipeline_options,
      bool is_major_updated) override {
    return nullptr;
  }

  void SendGlobalEvent(const std::string& event,
                       const lepus::Value& info) override {}
  void TriggerLepusGlobalEvent(const std::string& event,
                               const lepus::Value& info) override {}
  event::DispatchEventResult DispatchMessageEvent(
      fml::RefPtr<runtime::MessageEvent> event) override {
    return {event::EventCancelType::kNotCanceled, false};
  }
  bool EnableEventHandleRefactor() const override { return false; }
  bool SupportComponentJS() const override { return false; }
  runtime::MTSRuntime* GetDefaultEntryRuntime() const override {
    return nullptr;
  }
  runtime::MTSRuntime* GetEntryRuntime(
      const std::string& entry_name) const override {
    return nullptr;
  }
  std::string GetDefaultEntryLogicalName() const override { return {}; }
  EventResult FireElementWorkletAndRequestResolve(
      const std::string& component_id, const std::string& entry_name,
      const lepus::Value& callback, const lepus::Value& script,
      const lepus::Value& event_detail,
      const std::shared_ptr<worklet::LepusApiHandler>& task_handler,
      int32_t element_id,
      std::shared_ptr<PipelineOptions>& pipeline_options) override {
    return static_cast<EventResult>(0);
  }
  EventResult CallMTSClosureAndRequestResolve(
      runtime::MTSRuntime* runtime_context, const lepus::Value& callback,
      const lepus::Value& event_detail,
      std::shared_ptr<PipelineOptions>& pipeline_options) override {
    return static_cast<EventResult>(0);
  }
  void OnLayoutAfter(PipelineLayoutData& data) override {}
};

struct BenchmarkEnvironment {
  explicit BenchmarkEnvironment(bool new_styling_pipeline = false,
                                bool level_order_parallel = false) {
    LynxEnvConfig lynx_env_config(kBenchmarkWidth, kBenchmarkHeight,
                                  kBenchmarkLayoutsUnitPerPx,
                                  kBenchmarkPhysicalPixelsPerLayoutUnit);
    delegate = std::make_unique<NoOpDelegate>();
    element_manager_delegate = std::make_unique<NoOpElementManagerDelegate>();
    element_manager = std::make_unique<ElementManager>(
        std::make_unique<NoOpPaintingContext>(), delegate.get(),
        lynx_env_config);
    element_manager->SetElementManagerDelegate(element_manager_delegate.get());

    config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    config->SetEnableStandardCSSSelector(true);
    config->SetEnableCSSInvalidation(true);
    config->SetEnableNewStylingPipeline(new_styling_pipeline);
    config->SetEnableCSSInheritance(true);
    config->SetEnableCSSInlineVariables(true);
    element_manager->SetConfig(config);
    element_manager->SetEnableParallelElement(level_order_parallel);
    element_manager->SetEnableLevelOrderTraversing(level_order_parallel);
  }

  std::unique_ptr<NoOpElementManagerDelegate> element_manager_delegate;
  std::unique_ptr<ElementManager> element_manager;
  std::unique_ptr<NoOpDelegate> delegate;
  std::shared_ptr<PageConfig> config;
};

class BenchmarkFiberElement : public Element {
 public:
  explicit BenchmarkFiberElement(ElementManager* manager)
      : Element(manager, base::String("view")) {}

  bool IsStyleDirtyForBenchmark() const { return StyleDirty(); }
  void ClearDirtyForBenchmark() { ResetAllDirtyBits(); }
  CSSValue CurrentStyleForBenchmark(CSSPropertyID id) {
    const auto& resolved_values = computed_css_style()->GetResolvedValues();
    const auto it = resolved_values.find(id);
    if (it != resolved_values.end()) {
      return it->second;
    }
    return ResolveCurrentStyleValue(id, CSSValue());
  }
};

struct FiberTree {
  fml::RefPtr<PageElement> page;
  std::vector<fml::RefPtr<BenchmarkFiberElement>> nodes;

  BenchmarkFiberElement* root() const {
    return nodes.empty() ? nullptr : nodes.front().get();
  }
};

inline std::unique_ptr<css::LynxCSSSelector[]> ParseSelector(
    const std::string& selector_text) {
  css::CSSParserContext context;
  css::CSSTokenizer tokenizer(selector_text);
  const auto tokens = tokenizer.TokenizeToEOF();
  css::CSSParserTokenRange range(tokens);
  auto selector_vector = css::CSSSelectorParser::ParseSelector(range, &context);
  const size_t flattened_size =
      css::CSSSelectorParser::FlattenedSize(selector_vector);
  if (flattened_size == 0) {
    return nullptr;
  }
  auto selector_array =
      std::make_unique<css::LynxCSSSelector[]>(flattened_size);
  css::CSSSelectorParser::AdoptSelectorVector(
      selector_vector, selector_array.get(), flattened_size);
  return selector_array;
}

inline CSSValue ParseVariableValue(const std::string& raw_value,
                                   const CSSParserConfigs& configs) {
  CSSStringParser parser(raw_value.c_str(),
                         static_cast<uint32_t>(raw_value.length()), configs);
  return parser.ParseVariable();
}

inline std::unique_ptr<SharedCSSFragment> CreateFragment(int32_t id) {
  auto fragment = std::make_unique<SharedCSSFragment>(id);
  fragment->SetEnableCSSInvalidation();
  fragment->SetEnableCSSSelector();
  return fragment;
}

inline void AddRule(SharedCSSFragment& fragment,
                    const std::string& selector_text, StyleMap styles = {},
                    CSSVariableMap variables = {}) {
  auto selector = ParseSelector(selector_text);
  if (!selector) {
    return;
  }
  CSSParserConfigs configs;
  auto token = fml::MakeRefCounted<CSSParseToken>(configs);
  token->SetAttributes(std::move(styles));
  token->SetStyleVariables(std::move(variables));
  fragment.AddStyleRule(std::move(selector), std::move(token));
}

inline std::shared_ptr<CSSStyleSheetManager> InstallIntrinsicStyleSheet(
    PageElement& page, std::unique_ptr<SharedCSSFragment> fragment) {
  auto style_sheet_manager = std::make_shared<CSSStyleSheetManager>(nullptr);
  style_sheet_manager->AddSharedCSSFragment(std::move(fragment));
  page.set_style_sheet_manager(style_sheet_manager);
  return style_sheet_manager;
}

inline FiberTree BuildBalancedTree(ElementManager& manager, size_t node_count,
                                   size_t target_stride = 0) {
  FiberTree tree;
  tree.page = manager.CreateFiberPage("benchmark-page", kBenchmarkCSSId);
  tree.nodes.reserve(node_count);
  for (size_t i = 0; i < node_count; ++i) {
    auto node = fml::AdoptRef<BenchmarkFiberElement>(
        new BenchmarkFiberElement(&manager));
    node->SetParentComponentUniqueIdForFiber(
        static_cast<int64_t>(tree.page->impl_id()));
    node->MarkAttached();
    if (target_stride != 0 && i % target_stride == 0) {
      node->SetClass("target");
    }
    if (i == 0) {
      tree.page->InsertNode(node);
    } else {
      tree.nodes[(i - 1) / 2]->InsertNode(node);
    }
    tree.nodes.emplace_back(std::move(node));
  }
  return tree;
}

inline lepus::Value CSSVariableUpdate(const char* name, const char* value) {
  auto table = lepus::Dictionary::Create();
  table->SetValue(name, lepus::Value(value));
  return lepus::Value(std::move(table));
}

}  // namespace benchmark_support
}  // namespace tasm
}  // namespace lynx

#endif  // TESTING_TELEMETRY_STYLING_CSS_STYLING_BENCHMARK_SUPPORT_H_
