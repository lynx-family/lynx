// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sstream>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "testing/telemetry/styling/css/styling_benchmark_support.h"

namespace lynx {
namespace tasm {

namespace {

using benchmark_support::BenchmarkEnvironment;

// Helper to create a CSSValue with variable references using CSSStringParser
CSSValue CreateVariableValue(const std::string& raw_value) {
  CSSParserConfigs configs;
  return benchmark_support::ParseVariableValue(raw_value, configs);
}

}  // namespace

static void BM_GetCSSVariableByRule(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  auto element = fml::AdoptRef(new Element(env.element_manager.get(), "view"));
  AttributeHolder* holder = element->data_model();
  holder->UpdateCSSVariable("--main-bg-color", "red");
  holder->UpdateCSSVariable("--main-width", "100px");

  base::String default_props;
  lepus::Value default_value_map;
  std::string rule = "calc({{--main-width}} * 2)";

  for (auto _ : state) {
    auto result = handler.GetCSSVariableByRule(rule, holder, default_props,
                                               default_value_map);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_GetCSSVariableByRule);

static void BM_HandleCSSVariables_Simple(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  auto element = fml::AdoptRef(new Element(env.element_manager.get(), "view"));
  AttributeHolder* holder = element->data_model();
  holder->UpdateCSSVariable("--bg-color", "blue");

  CSSParserConfigs configs;
  StyleMap original_map;

  // Prepare a style map with one variable
  original_map.insert_or_assign(kPropertyIDBackgroundColor,
                                CreateVariableValue("var(--bg-color)"));

  element->CollectCustomProperties(holder);

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      const bool resolved =
          handler.HandleCSSVariables(maps[i], holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}

BENCHMARK(BM_HandleCSSVariables_Simple);

static void BM_HandleCSSVariables_DeeplyNested(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  auto element = fml::AdoptRef(new Element(env.element_manager.get(), "view"));
  AttributeHolder* holder = element->data_model();

  int depth = state.range(0);

  // Create a chain: --var0 -> --var1 -> ... -> --varN -> red
  for (int i = 0; i < depth; ++i) {
    std::string current = "--var" + std::to_string(i);
    std::string next =
        (i == depth - 1) ? "red" : "var(--var" + std::to_string(i + 1) + ")";
    holder->UpdateCSSVariable(current, next);
  }

  CSSParserConfigs configs;
  StyleMap original_map;
  original_map.insert_or_assign(kPropertyIDColor,
                                CreateVariableValue("var(--var0)"));

  element->CollectCustomProperties(holder);

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      const bool resolved =
          handler.HandleCSSVariables(maps[i], holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}
BENCHMARK(BM_HandleCSSVariables_DeeplyNested)->Range(1, 16);

static void BM_HandleCSSVariables_LargeScope(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  auto element = fml::AdoptRef(new Element(env.element_manager.get(), "view"));
  AttributeHolder* holder = element->data_model();

  int num_vars = state.range(0);
  // Populate many variables
  for (int i = 0; i < num_vars; ++i) {
    holder->UpdateCSSVariable("--unused-" + std::to_string(i), "10px");
  }
  holder->UpdateCSSVariable("--target", "red");

  CSSParserConfigs configs;
  StyleMap original_map;
  original_map.insert_or_assign(kPropertyIDColor,
                                CreateVariableValue("var(--target)"));

  element->CollectCustomProperties(holder);

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      const bool resolved =
          handler.HandleCSSVariables(maps[i], holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}
BENCHMARK(BM_HandleCSSVariables_LargeScope)->Range(10, 1000);

static void BM_HandleCSSVariables_Fallback(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  auto element = fml::AdoptRef(new Element(env.element_manager.get(), "view"));
  AttributeHolder* holder = element->data_model();
  // No variables defined in holder

  CSSParserConfigs configs;
  StyleMap original_map;

  original_map.insert_or_assign(kPropertyIDColor,
                                CreateVariableValue("var(--missing, blue)"));

  element->CollectCustomProperties(holder);

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      const bool resolved =
          handler.HandleCSSVariables(maps[i], holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}
BENCHMARK(BM_HandleCSSVariables_Fallback);

static void BM_HandleCSSVariables_MultiVars(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  auto element = fml::AdoptRef(new Element(env.element_manager.get(), "view"));
  AttributeHolder* holder = element->data_model();
  holder->UpdateCSSVariable("--width", "10px");
  holder->UpdateCSSVariable("--color", "red");

  CSSParserConfigs configs;
  StyleMap original_map;

  // border: var(--width) solid var(--color)
  original_map.insert_or_assign(
      kPropertyIDBorder,
      CreateVariableValue("var(--width) solid var(--color)"));

  element->CollectCustomProperties(holder);

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      const bool resolved =
          handler.HandleCSSVariables(maps[i], holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}
BENCHMARK(BM_HandleCSSVariables_MultiVars);

static void BM_HandleCSSVariables_Cascade(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  int depth = state.range(0);

  std::vector<fml::RefPtr<Element>> elements;
  for (int i = 0; i < depth; ++i) {
    elements.push_back(
        fml::AdoptRef(new Element(env.element_manager.get(), "view")));
    if (i > 0) {
      elements[i - 1]->InsertNode(elements[i]);
    }
  }

  // Define a variable at the root
  elements[0]->data_model()->UpdateCSSVariable("--main-color", "red");

  auto leaf = elements.back();
  auto leaf_holder = leaf->data_model();

  CSSParserConfigs configs;
  StyleMap original_map;
  // Use the variable defined at the root
  original_map.insert_or_assign(kPropertyIDColor,
                                CreateVariableValue("var(--main-color)"));

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      // Collect custom properties to simulate the cascading process
      state.PauseTiming();
      for (auto& el : elements) {
        el->MarkCustomPropertiesDirty();
      }
      state.ResumeTiming();

      leaf->CollectCustomProperties(leaf_holder);
      const bool resolved =
          handler.HandleCSSVariables(maps[i], leaf_holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}
BENCHMARK(BM_HandleCSSVariables_Cascade)->Range(1, 64);

static void BM_HandleCSSVariables_Redefinition(benchmark::State& state) {
  BenchmarkEnvironment env;
  CSSVariableHandler handler(true);
  int depth = state.range(0);

  std::vector<fml::RefPtr<Element>> elements;
  for (int i = 0; i < depth; ++i) {
    elements.push_back(
        fml::AdoptRef(new Element(env.element_manager.get(), "view")));
    if (i > 0) {
      elements[i - 1]->InsertNode(elements[i]);
    }
    // Redefine the same variable at each level
    elements[i]->data_model()->UpdateCSSVariable("--color", "red");
  }

  auto leaf = elements.back();
  auto leaf_holder = leaf->data_model();

  CSSParserConfigs configs;
  StyleMap original_map;
  original_map.insert_or_assign(kPropertyIDColor,
                                CreateVariableValue("var(--color)"));

  const int kBatchSize = 100;
  std::vector<StyleMap> maps(kBatchSize, original_map);

  for (auto _ : state) {
    for (int i = 0; i < kBatchSize; ++i) {
      state.PauseTiming();
      for (auto& el : elements) {
        el->MarkCustomPropertiesDirty();
      }
      state.ResumeTiming();

      leaf->CollectCustomProperties(leaf_holder);
      const bool resolved =
          handler.HandleCSSVariables(maps[i], leaf_holder, configs);
      benchmark::DoNotOptimize(resolved);
    }
    state.PauseTiming();
    for (int i = 0; i < kBatchSize; ++i) {
      maps[i] = original_map;
    }
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * kBatchSize);
}
BENCHMARK(BM_HandleCSSVariables_Redefinition)->Range(1, 64);

}  // namespace tasm
}  // namespace lynx
