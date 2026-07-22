// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>
#include <memory>
#include <string>

#include "benchmark/benchmark.h"
#include "core/renderer/css/ng/matcher/selector_matcher.h"
#include "core/renderer/tasm/testing/mock_attribute_holder.h"
#include "testing/telemetry/styling/css/styling_benchmark_support.h"

namespace lynx {
namespace css {
namespace {

namespace bs = tasm::benchmark_support;

struct MatcherFixture {
  std::unique_ptr<tasm::MockAttributeHolder> tree;
  tasm::MockAttributeHolder* target = nullptr;
};

MatcherFixture BuildDeepFixture(size_t depth, bool matching_ancestor) {
  MatcherFixture fixture;
  fixture.tree = std::make_unique<tasm::MockAttributeHolder>("view");
  if (matching_ancestor) {
    fixture.tree->SetClass("ancestor");
  }

  auto* parent = fixture.tree.get();
  for (size_t i = 1; i < depth; ++i) {
    auto child = std::make_unique<tasm::MockAttributeHolder>("view");
    auto* child_ptr = child.get();
    parent->AddChild(std::move(child));
    parent = child_ptr;
  }
  auto target = std::make_unique<tasm::MockAttributeHolder>("view");
  fixture.target = target.get();
  fixture.target->SetClass("target");
  parent->AddChild(std::move(target));
  return fixture;
}

MatcherFixture BuildSiblingFixture(size_t width, bool matching_sibling) {
  MatcherFixture fixture;
  fixture.tree = std::make_unique<tasm::MockAttributeHolder>("view");
  for (size_t i = 0; i + 1 < width; ++i) {
    auto sibling = std::make_unique<tasm::MockAttributeHolder>("view");
    if (i == 0 && matching_sibling) {
      sibling->SetClass("anchor");
    }
    fixture.tree->AddChild(std::move(sibling));
  }
  auto target = std::make_unique<tasm::MockAttributeHolder>("view");
  fixture.target = target.get();
  fixture.target->SetClass("target");
  fixture.tree->AddChild(std::move(target));
  return fixture;
}

void RunMatcher(benchmark::State& state, const std::string& selector_text,
                StyleNode* target, bool expected) {
  auto selector = bs::ParseSelector(selector_text);
  if (!selector) {
    state.SkipWithError("failed to parse benchmark selector");
    return;
  }
  SelectorMatcher matcher;
  SelectorMatcher::SelectorMatchingContext context(target);
  context.selector = selector.get();
  if (matcher.Match(context) != expected) {
    state.SkipWithError("selector fixture has unexpected match result");
    return;
  }

  for (auto _ : state) {
    const bool matched = matcher.Match(context);
    benchmark::DoNotOptimize(matched);
  }
  state.SetItemsProcessed(state.iterations());
}

void BM_SelectorMatcherSimple(benchmark::State& state, const char* selector,
                              bool expected) {
  tasm::MockAttributeHolder target("view");
  target.SetClass("target");
  target.SetIdSelector("main");
  target.SetPseudoState(tasm::kPseudoStateFocus);
  RunMatcher(state, selector, &target, expected);
}

void BM_SelectorMatcherDescendant(benchmark::State& state, bool expected) {
  const size_t depth = static_cast<size_t>(state.range(0));
  auto fixture = BuildDeepFixture(depth, expected);
  RunMatcher(state, ".ancestor .target", fixture.target, expected);
  state.counters["Depth"] = static_cast<double>(depth);
}

void BM_SelectorMatcherChild(benchmark::State& state, bool expected) {
  auto fixture = BuildDeepFixture(1, expected);
  RunMatcher(state, ".ancestor > .target", fixture.target, expected);
}

void BM_SelectorMatcherGeneralSibling(benchmark::State& state, bool expected) {
  const size_t width = static_cast<size_t>(state.range(0));
  auto fixture = BuildSiblingFixture(width, expected);
  RunMatcher(state, ".anchor ~ .target", fixture.target, expected);
  state.counters["Width"] = static_cast<double>(width);
}

void BM_SelectorMatcherDirectAdjacent(benchmark::State& state, bool expected) {
  MatcherFixture fixture;
  fixture.tree = std::make_unique<tasm::MockAttributeHolder>("view");
  auto previous = std::make_unique<tasm::MockAttributeHolder>("view");
  if (expected) {
    previous->SetClass("anchor");
  }
  fixture.tree->AddChild(std::move(previous));
  auto target = std::make_unique<tasm::MockAttributeHolder>("view");
  fixture.target = target.get();
  fixture.target->SetClass("target");
  fixture.tree->AddChild(std::move(target));
  RunMatcher(state, ".anchor + .target", fixture.target, expected);
}

BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, ClassHit, ".target", true);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, ClassMiss, ".missing", false);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, IdHit, "#main", true);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, IdMiss, "#missing", false);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, TagHit, "view", true);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, TagMiss, "text", false);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, CompoundHit,
                  "view.target:focus:not(.disabled)", true);
BENCHMARK_CAPTURE(BM_SelectorMatcherSimple, CompoundMiss,
                  "view.target:focus:not(.target)", false);

BENCHMARK_CAPTURE(BM_SelectorMatcherDescendant, Hit, true)
    ->Arg(1)
    ->Arg(8)
    ->Arg(32);
BENCHMARK_CAPTURE(BM_SelectorMatcherDescendant, Miss, false)
    ->Arg(1)
    ->Arg(8)
    ->Arg(32);
BENCHMARK_CAPTURE(BM_SelectorMatcherChild, Hit, true);
BENCHMARK_CAPTURE(BM_SelectorMatcherChild, Miss, false);
BENCHMARK_CAPTURE(BM_SelectorMatcherGeneralSibling, Hit, true)
    ->Arg(8)
    ->Arg(64)
    ->Arg(512);
BENCHMARK_CAPTURE(BM_SelectorMatcherGeneralSibling, Miss, false)
    ->Arg(8)
    ->Arg(64)
    ->Arg(512);
BENCHMARK_CAPTURE(BM_SelectorMatcherDirectAdjacent, Hit, true);
BENCHMARK_CAPTURE(BM_SelectorMatcherDirectAdjacent, Miss, false);

}  // namespace
}  // namespace css
}  // namespace lynx
