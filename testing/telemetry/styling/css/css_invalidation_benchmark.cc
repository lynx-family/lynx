// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>
#include <string>

#include "benchmark/benchmark.h"
#include "core/renderer/css/ng/invalidation/rule_invalidation_set.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"
#include "core/renderer/css/ng/style/style_rule.h"
#include "testing/telemetry/styling/css/styling_benchmark_support.h"

namespace lynx {
namespace css {
namespace {

namespace bs = tasm::benchmark_support;

enum class InvalidationKey { kClass, kId, kPseudo };

void AddSelector(RuleInvalidationSet& invalidation_set,
                 const std::string& selector_text) {
  StyleRule rule(bs::ParseSelector(selector_text), nullptr);
  for (const LynxCSSSelector* selector = rule.FirstSelector(); selector;
       selector = LynxCSSSelectorList::Next(*selector)) {
    invalidation_set.AddSelector(*selector);
  }
}

void BM_RuleInvalidationCollect(benchmark::State& state, InvalidationKey key) {
  const int feature_count = static_cast<int>(state.range(0));
  RuleInvalidationSet invalidation_set;
  for (int i = 0; i < feature_count; ++i) {
    const std::string target = ".target-" + std::to_string(i);
    switch (key) {
      case InvalidationKey::kClass:
        AddSelector(invalidation_set, ".ancestor " + target);
        break;
      case InvalidationKey::kId:
        AddSelector(invalidation_set, "#ancestor " + target);
        break;
      case InvalidationKey::kPseudo:
        AddSelector(invalidation_set, ":hover " + target);
        break;
    }
  }

  for (auto _ : state) {
    InvalidationLists lists;
    switch (key) {
      case InvalidationKey::kClass:
        invalidation_set.CollectClass(lists, "ancestor");
        break;
      case InvalidationKey::kId:
        invalidation_set.CollectId(lists, "ancestor");
        break;
      case InvalidationKey::kPseudo:
        invalidation_set.CollectPseudoClass(
            lists, LynxCSSSelector::PseudoType::kPseudoHover);
        break;
    }
    benchmark::DoNotOptimize(lists.descendants.data());
    benchmark::ClobberMemory();
  }
  state.counters["Features"] = feature_count;
  state.SetItemsProcessed(state.iterations());
}

void BM_FiberClassInvalidation(benchmark::State& state) {
  const size_t node_count = static_cast<size_t>(state.range(0));
  const size_t target_stride = static_cast<size_t>(state.range(1));

  bs::BenchmarkEnvironment env;
  auto fragment = bs::CreateFragment(bs::kBenchmarkCSSId);
  bs::AddRule(*fragment, ".state-a .target");
  bs::AddRule(*fragment, ".state-b .target");

  auto tree =
      bs::BuildBalancedTree(*env.element_manager, node_count, target_stride);
  auto style_sheet_manager =
      bs::InstallIntrinsicStyleSheet(*tree.page, std::move(fragment));
  benchmark::DoNotOptimize(style_sheet_manager.get());

  for (auto& node : tree.nodes) {
    node->ClearDirtyForBenchmark();
  }

  const size_t expected_dirty = [&]() {
    if (target_stride == 0) {
      return size_t{0};
    }
    size_t count = 0;
    // The changed element is not part of its own descendant invalidation.
    for (size_t i = 1; i < node_count; ++i) {
      count += i % target_stride == 0 ? 1 : 0;
    }
    return count;
  }();

  const tasm::ClassList state_a = {base::String("state-a")};
  const tasm::ClassList state_b = {base::String("state-b")};
  bool use_state_a = true;
  for (auto _ : state) {
    const tasm::ClassList& old_classes = use_state_a ? state_a : state_b;
    const tasm::ClassList& new_classes = use_state_a ? state_b : state_a;
    tree.root()->OnClassChanged(old_classes, new_classes);
    tree.root()->InvalidateChildrenIfNeeded();

    state.PauseTiming();
    size_t dirty_count = 0;
    for (auto& node : tree.nodes) {
      dirty_count += node->IsStyleDirtyForBenchmark() ? 1 : 0;
      node->ClearDirtyForBenchmark();
    }
    if (dirty_count != expected_dirty) {
      state.ResumeTiming();
      state.SkipWithError("unexpected invalidated Fiber node count");
      break;
    }
    use_state_a = !use_state_a;
    state.ResumeTiming();
  }

  state.counters["Nodes"] = static_cast<double>(node_count);
  state.counters["AffectedNodes"] = static_cast<double>(expected_dirty);
  state.counters["TargetStride"] = static_cast<double>(target_stride);
  state.SetItemsProcessed(state.iterations() * node_count);
}

BENCHMARK_CAPTURE(BM_RuleInvalidationCollect, Class, InvalidationKey::kClass)
    ->Arg(1)
    ->Arg(16)
    ->Arg(256);
BENCHMARK_CAPTURE(BM_RuleInvalidationCollect, Id, InvalidationKey::kId)
    ->Arg(1)
    ->Arg(16)
    ->Arg(256);
BENCHMARK_CAPTURE(BM_RuleInvalidationCollect, Pseudo, InvalidationKey::kPseudo)
    ->Arg(1)
    ->Arg(16)
    ->Arg(256);

BENCHMARK(BM_FiberClassInvalidation)
    ->Args({127, 0})
    ->Args({127, 8})
    ->Args({127, 1})
    ->Args({1023, 0})
    ->Args({1023, 8})
    ->Args({1023, 1})
    ->Args({4095, 0})
    ->Args({4095, 8})
    ->Args({4095, 1});

}  // namespace
}  // namespace css
}  // namespace lynx
