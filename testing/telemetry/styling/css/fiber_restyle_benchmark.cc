// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "benchmark/benchmark.h"
#include "core/runtime/lepus/bindings/style/shared_css_fragment_wrapper.h"
#include "testing/telemetry/styling/css/styling_benchmark_support.h"

namespace lynx {
namespace tasm {
namespace {

namespace bs = benchmark_support;

enum class ClassScenario { kSelf, kDescendantSparse, kWholeSubtree };
enum class VariableScenario { kSparse, kDense, kUnrelated };

StyleMap WidthStyle(double width) {
  StyleMap styles;
  styles.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                          CSSValue(width, CSSValuePattern::PX));
  return styles;
}

bool HasWidth(bs::BenchmarkFiberElement& element, double expected) {
  const CSSValue value =
      element.CurrentStyleForBenchmark(CSSPropertyID::kPropertyIDWidth);
  return value.GetPattern() == CSSValuePattern::PX &&
         value.AsNumber() == expected;
}

void ChangeClass(bs::FiberTree& tree, const char* class_name) {
  const ClassList old_classes = tree.root()->classes();
  ClassList new_classes = {base::String(class_name)};
  tree.root()->OnClassChanged(old_classes, new_classes);
  tree.root()->SetClasses(std::move(new_classes));
}

bs::BenchmarkFiberElement* FirstDescendantTarget(bs::FiberTree& tree,
                                                 size_t target_stride) {
  if (target_stride == 0) {
    return nullptr;
  }
  for (size_t i = 1; i < tree.nodes.size(); ++i) {
    if (i % target_stride == 0) {
      return tree.nodes[i].get();
    }
  }
  return nullptr;
}

size_t DescendantTargetCount(size_t node_count, size_t target_stride) {
  if (target_stride == 0) {
    return 0;
  }
  size_t result = 0;
  for (size_t i = 1; i < node_count; ++i) {
    result += i % target_stride == 0 ? 1 : 0;
  }
  return result;
}

void BM_FiberRestyleClass(benchmark::State& state, bool new_pipeline,
                          bool parallel, ClassScenario scenario) {
  const size_t node_count = static_cast<size_t>(state.range(0));
  const size_t target_stride =
      scenario == ClassScenario::kDescendantSparse ? 8 : 0;

  bs::BenchmarkEnvironment env(new_pipeline, parallel);
  auto fragment = bs::CreateFragment(bs::kBenchmarkCSSId);
  switch (scenario) {
    case ClassScenario::kSelf:
      bs::AddRule(*fragment, ".state-a", WidthStyle(10));
      bs::AddRule(*fragment, ".state-b", WidthStyle(20));
      break;
    case ClassScenario::kDescendantSparse:
      bs::AddRule(*fragment, ".state-a .target", WidthStyle(10));
      bs::AddRule(*fragment, ".state-b .target", WidthStyle(20));
      break;
    case ClassScenario::kWholeSubtree:
      bs::AddRule(*fragment, ".state-a *", WidthStyle(10));
      bs::AddRule(*fragment, ".state-b *", WidthStyle(20));
      break;
  }

  auto tree =
      bs::BuildBalancedTree(*env.element_manager, node_count, target_stride);
  auto style_sheet_manager =
      bs::InstallIntrinsicStyleSheet(*tree.page, std::move(fragment));
  benchmark::DoNotOptimize(style_sheet_manager.get());

  ChangeClass(tree, "state-a");
  tree.page->FlushActionsAsRoot();
  bs::BenchmarkFiberElement* verification_target =
      scenario == ClassScenario::kDescendantSparse
          ? FirstDescendantTarget(tree, target_stride)
      : scenario == ClassScenario::kWholeSubtree ? tree.nodes[1].get()
                                                 : tree.root();
  if (verification_target == nullptr || !HasWidth(*verification_target, 10)) {
    state.SkipWithError("initial class restyle did not resolve expected width");
    return;
  }

  // Validate both alternating states before entering the timed loop.
  ChangeClass(tree, "state-b");
  tree.page->FlushActionsAsRoot();
  if (!HasWidth(*verification_target, 20)) {
    state.SkipWithError("class restyle did not resolve state-b width");
    return;
  }
  ChangeClass(tree, "state-a");
  tree.page->FlushActionsAsRoot();

  bool switch_to_b = true;
  for (auto _ : state) {
    ChangeClass(tree, switch_to_b ? "state-b" : "state-a");
    tree.page->FlushActionsAsRoot();
    benchmark::DoNotOptimize(verification_target->computed_css_style());
    switch_to_b = !switch_to_b;
  }

  size_t affected_nodes = 1;
  if (scenario == ClassScenario::kDescendantSparse) {
    affected_nodes += DescendantTargetCount(node_count, target_stride);
  } else if (scenario == ClassScenario::kWholeSubtree) {
    affected_nodes = node_count;
  }
  state.counters["Nodes"] = static_cast<double>(node_count);
  state.counters["AffectedNodes"] = static_cast<double>(affected_nodes);
  state.counters["Parallel"] = parallel ? 1 : 0;
  state.counters["NewPipeline"] = new_pipeline ? 1 : 0;
  state.SetItemsProcessed(state.iterations() * affected_nodes);
}

void BM_FiberRestyleRuleVolume(benchmark::State& state, bool new_pipeline) {
  constexpr size_t kNodeCount = 1023;
  constexpr size_t kTargetStride = 8;
  const int rule_count = static_cast<int>(state.range(0));

  bs::BenchmarkEnvironment env(new_pipeline);
  auto fragment = bs::CreateFragment(bs::kBenchmarkCSSId);
  for (int i = 0; i < rule_count; ++i) {
    bs::AddRule(*fragment, ".ancestor-" + std::to_string(i) + " .target",
                WidthStyle(i + 1));
  }
  auto tree =
      bs::BuildBalancedTree(*env.element_manager, kNodeCount, kTargetStride);
  auto style_sheet_manager =
      bs::InstallIntrinsicStyleSheet(*tree.page, std::move(fragment));
  benchmark::DoNotOptimize(style_sheet_manager.get());

  auto* verification_target = FirstDescendantTarget(tree, kTargetStride);
  ChangeClass(tree, "ancestor-0");
  tree.page->FlushActionsAsRoot();
  if (verification_target == nullptr || !HasWidth(*verification_target, 1)) {
    state.SkipWithError("initial rule-volume style did not resolve");
    return;
  }
  ChangeClass(tree, "ancestor-1");
  tree.page->FlushActionsAsRoot();
  if (!HasWidth(*verification_target, 2)) {
    state.SkipWithError("rule-volume state-b style did not resolve");
    return;
  }
  ChangeClass(tree, "ancestor-0");
  tree.page->FlushActionsAsRoot();

  bool switch_to_second = true;
  for (auto _ : state) {
    ChangeClass(tree, switch_to_second ? "ancestor-1" : "ancestor-0");
    tree.page->FlushActionsAsRoot();
    benchmark::DoNotOptimize(verification_target->computed_css_style());
    switch_to_second = !switch_to_second;
  }
  state.counters["Nodes"] = static_cast<double>(kNodeCount);
  state.counters["Rules"] = rule_count;
  state.counters["AffectedNodes"] =
      static_cast<double>(DescendantTargetCount(kNodeCount, kTargetStride) + 1);
  state.counters["NewPipeline"] = new_pipeline ? 1 : 0;
  state.SetItemsProcessed(
      state.iterations() *
      (DescendantTargetCount(kNodeCount, kTargetStride) + 1));
}

void UpdateVariable(bs::FiberTree& tree,
                    std::shared_ptr<PipelineOptions>& options,
                    const lepus::Value& update) {
  tree.root()->UpdateCSSVariable(update, options);
  tree.page->FlushActionsAsRoot();
}

void BM_FiberRestyleCSSVariable(benchmark::State& state, bool new_pipeline,
                                bool parallel, VariableScenario scenario) {
  const size_t node_count = static_cast<size_t>(state.range(0));
  const size_t target_stride = scenario == VariableScenario::kDense ? 1 : 8;

  bs::BenchmarkEnvironment env(new_pipeline, parallel);
  auto fragment = bs::CreateFragment(bs::kBenchmarkCSSId);
  CSSParserConfigs configs;
  StyleMap variable_style;
  variable_style.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth,
      bs::ParseVariableValue("var(--bench-size)", configs));
  bs::AddRule(*fragment, ".target", std::move(variable_style));

  auto tree =
      bs::BuildBalancedTree(*env.element_manager, node_count, target_stride);
  auto style_sheet_manager =
      bs::InstallIntrinsicStyleSheet(*tree.page, std::move(fragment));
  benchmark::DoNotOptimize(style_sheet_manager.get());

  auto options = std::make_shared<PipelineOptions>();
  options->enable_unified_pixel_pipeline = true;
  const lepus::Value size_10 = bs::CSSVariableUpdate("--bench-size", "10px");
  const lepus::Value size_20 = bs::CSSVariableUpdate("--bench-size", "20px");
  const lepus::Value unused_alpha =
      bs::CSSVariableUpdate("--bench-unused", "alpha");
  const lepus::Value unused_beta =
      bs::CSSVariableUpdate("--bench-unused", "beta");

  UpdateVariable(tree, options, size_10);
  if (scenario == VariableScenario::kUnrelated) {
    UpdateVariable(tree, options, unused_alpha);
  }

  auto* verification_target = tree.root();
  if (!HasWidth(*verification_target, 10)) {
    state.SkipWithError("initial CSS variable style did not resolve");
    return;
  }

  const lepus::Value& second_update =
      scenario == VariableScenario::kUnrelated ? unused_beta : size_20;
  const lepus::Value& first_update =
      scenario == VariableScenario::kUnrelated ? unused_alpha : size_10;

  UpdateVariable(tree, options, second_update);
  if (!HasWidth(*verification_target,
                scenario == VariableScenario::kUnrelated ? 10 : 20)) {
    state.SkipWithError("CSS variable state-b style did not resolve");
    return;
  }
  UpdateVariable(tree, options, first_update);

  bool switch_to_second = true;
  for (auto _ : state) {
    UpdateVariable(tree, options,
                   switch_to_second ? second_update : first_update);
    benchmark::DoNotOptimize(verification_target->computed_css_style());
    switch_to_second = !switch_to_second;
  }

  size_t affected_nodes = 1;
  if (scenario != VariableScenario::kUnrelated) {
    affected_nodes = target_stride == 1
                         ? node_count
                         : DescendantTargetCount(node_count, target_stride) + 1;
  }
  state.counters["Nodes"] = static_cast<double>(node_count);
  state.counters["AffectedNodes"] = static_cast<double>(affected_nodes);
  state.counters["Parallel"] = parallel ? 1 : 0;
  state.counters["NewPipeline"] = new_pipeline ? 1 : 0;
  state.SetItemsProcessed(state.iterations() * affected_nodes);
}

void BM_FiberRestyleAdoptedStylesheets(benchmark::State& state,
                                       bool new_pipeline) {
  constexpr size_t kNodeCount = 1023;
  constexpr size_t kTargetStride = 8;
  const int adopted_count = static_cast<int>(state.range(0));

  bs::BenchmarkEnvironment env(new_pipeline);
  auto intrinsic = bs::CreateFragment(bs::kBenchmarkCSSId);
  auto tree =
      bs::BuildBalancedTree(*env.element_manager, kNodeCount, kTargetStride);
  auto style_sheet_manager =
      bs::InstallIntrinsicStyleSheet(*tree.page, std::move(intrinsic));
  benchmark::DoNotOptimize(style_sheet_manager.get());

  for (int i = 0; i < adopted_count; ++i) {
    auto adopted = bs::CreateFragment(-1 - i);
    bs::AddRule(*adopted, ".state-a .target", WidthStyle(10));
    bs::AddRule(*adopted, ".state-b .target", WidthStyle(20));
    env.element_manager->AdoptStyleSheet(
        fml::AdoptRef<SharedCSSFragmentWrapper>(
            new SharedCSSFragmentWrapper(std::move(adopted))));
  }

  auto* verification_target = FirstDescendantTarget(tree, kTargetStride);
  ChangeClass(tree, "state-a");
  tree.page->FlushActionsAsRoot();
  if (verification_target == nullptr || !HasWidth(*verification_target, 10)) {
    state.SkipWithError("adopted stylesheet state-a did not resolve");
    return;
  }
  ChangeClass(tree, "state-b");
  tree.page->FlushActionsAsRoot();
  if (!HasWidth(*verification_target, 20)) {
    state.SkipWithError("adopted stylesheet state-b did not resolve");
    return;
  }
  ChangeClass(tree, "state-a");
  tree.page->FlushActionsAsRoot();

  bool switch_to_b = true;
  for (auto _ : state) {
    ChangeClass(tree, switch_to_b ? "state-b" : "state-a");
    tree.page->FlushActionsAsRoot();
    benchmark::DoNotOptimize(verification_target->computed_css_style());
    switch_to_b = !switch_to_b;
  }
  state.counters["Nodes"] = static_cast<double>(kNodeCount);
  state.counters["AdoptedSheets"] = adopted_count;
  state.counters["AffectedNodes"] =
      static_cast<double>(DescendantTargetCount(kNodeCount, kTargetStride) + 1);
  state.counters["NewPipeline"] = new_pipeline ? 1 : 0;
  state.SetItemsProcessed(
      state.iterations() *
      (DescendantTargetCount(kNodeCount, kTargetStride) + 1));
}

#define REGISTER_SERIAL_CLASS(name, pipeline, scenario)                    \
  BENCHMARK_CAPTURE(BM_FiberRestyleClass, name, pipeline, false, scenario) \
      ->Arg(127)                                                           \
      ->Arg(1023)                                                          \
      ->Arg(4095)                                                          \
      ->UseRealTime()

REGISTER_SERIAL_CLASS(LegacySelf, false, ClassScenario::kSelf);
REGISTER_SERIAL_CLASS(NewSelf, true, ClassScenario::kSelf);
REGISTER_SERIAL_CLASS(LegacyDescendantSparse, false,
                      ClassScenario::kDescendantSparse);
REGISTER_SERIAL_CLASS(NewDescendantSparse, true,
                      ClassScenario::kDescendantSparse);
REGISTER_SERIAL_CLASS(LegacyWholeSubtree, false, ClassScenario::kWholeSubtree);
REGISTER_SERIAL_CLASS(NewWholeSubtree, true, ClassScenario::kWholeSubtree);

BENCHMARK_CAPTURE(BM_FiberRestyleRuleVolume, Legacy, false)
    ->Arg(32)
    ->Arg(256)
    ->Arg(1024)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleRuleVolume, New, true)
    ->Arg(32)
    ->Arg(256)
    ->Arg(1024)
    ->UseRealTime();

#define REGISTER_SERIAL_VARIABLE(name, pipeline, scenario)             \
  BENCHMARK_CAPTURE(BM_FiberRestyleCSSVariable, name, pipeline, false, \
                    scenario)                                          \
      ->Arg(127)                                                       \
      ->Arg(1023)                                                      \
      ->Arg(4095)                                                      \
      ->UseRealTime()

REGISTER_SERIAL_VARIABLE(LegacySparse, false, VariableScenario::kSparse);
REGISTER_SERIAL_VARIABLE(NewSparse, true, VariableScenario::kSparse);
REGISTER_SERIAL_VARIABLE(LegacyDense, false, VariableScenario::kDense);
REGISTER_SERIAL_VARIABLE(NewDense, true, VariableScenario::kDense);
REGISTER_SERIAL_VARIABLE(LegacyUnrelated, false, VariableScenario::kUnrelated);
REGISTER_SERIAL_VARIABLE(NewUnrelated, true, VariableScenario::kUnrelated);

BENCHMARK_CAPTURE(BM_FiberRestyleClass, LegacyParallelDescendantSparse, false,
                  true, ClassScenario::kDescendantSparse)
    ->Arg(4095)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleClass, NewParallelDescendantSparse, true, true,
                  ClassScenario::kDescendantSparse)
    ->Arg(4095)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleClass, LegacyParallelWholeSubtree, false, true,
                  ClassScenario::kWholeSubtree)
    ->Arg(4095)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleClass, NewParallelWholeSubtree, true, true,
                  ClassScenario::kWholeSubtree)
    ->Arg(4095)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleCSSVariable, LegacyParallelDense, false, true,
                  VariableScenario::kDense)
    ->Arg(4095)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleCSSVariable, NewParallelDense, true, true,
                  VariableScenario::kDense)
    ->Arg(4095)
    ->UseRealTime();

BENCHMARK_CAPTURE(BM_FiberRestyleAdoptedStylesheets, Legacy, false)
    ->Arg(1)
    ->Arg(8)
    ->UseRealTime();
BENCHMARK_CAPTURE(BM_FiberRestyleAdoptedStylesheets, New, true)
    ->Arg(1)
    ->Arg(8)
    ->UseRealTime();

#undef REGISTER_SERIAL_CLASS
#undef REGISTER_SERIAL_VARIABLE

}  // namespace
}  // namespace tasm
}  // namespace lynx
