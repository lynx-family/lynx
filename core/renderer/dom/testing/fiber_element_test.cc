// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/testing/fiber_element_test.h"

namespace lynx {
namespace tasm {
namespace testing {

void FiberElementTest::SetUp() {
  LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                kDefaultPhysicalPixelsPerLayoutUnit);
  vsync_monitor_ = std::make_shared<TestVSyncMonitor>();
  vsync_monitor_->BindToCurrentThread();
  auto unique_manager = std::make_unique<lynx::tasm::ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), &tasm_mediator,
      lynx_env_config, tasm::report::kUnknownInstanceId, vsync_monitor_);
  manager = unique_manager.get();
  platform_impl_ = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());

  tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
      tasm_mediator, std::move(unique_manager), 0);

  auto test_entry = std::make_shared<TemplateEntry>();
  tasm->template_entries_.insert({"test_entry", test_entry});

  auto config = std::make_shared<PageConfig>();
  config->SetEnableZIndex(true);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  tasm->page_config_ = config;

  thread_strategy = std::get<1>(current_parameter_);
  if (thread_strategy == 0) {
    manager->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager->SetThreadStrategy(base::ThreadStrategyForRendering::MULTI_THREADS);
  }
  enable_parallel_element_flush = std::get<0>(current_parameter_);
  manager->SetEnableParallelElement(enable_parallel_element_flush);
  const_cast<DynamicCSSConfigs&>(manager->GetDynamicCSSConfigs())
      .unify_vw_vh_behavior_ = true;
}

bool FiberElementTest::HasCapturePlatformNodeTag(int32_t target_id,
                                                 std::string expected_tag) {
  bool success = false;
  std::for_each(platform_impl_->captured_create_tags_map_.begin(),
                platform_impl_->captured_create_tags_map_.end(),
                [&](auto& pair) {
                  if (target_id == pair.first) {
                    success = pair.second == expected_tag;
                  }
                });
  return success;
}

bool FiberElementTest::HasCaptureSignWithLayoutAttribute(
    int32_t target_id, starlight::LayoutAttribute target_key,
    const lepus::Value& target_value, int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(tasm_mediator.captured_ids_.begin(),
                tasm_mediator.captured_ids_.end(), [&](int32_t id) {
                  if (target_id == id) {
                    auto& bundle = *(tasm_mediator.captured_bundles_)[index];
                    for (auto [key, value] : bundle.attrs) {
                      if (target_key == key &&
                          (value == target_value || target_value.IsEmpty())) {
                        --count;
                      }
                    }
                  }
                  ++index;
                });

  return count == 0;
}

bool FiberElementTest::HasCaptureSignWithStyleKeyAndValuePattern(
    int32_t target_id, CSSPropertyID target_key,
    const tasm::CSSValue& target_value, int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(
      tasm_mediator.captured_ids_.begin(), tasm_mediator.captured_ids_.end(),
      [&](int32_t id) {
        if (target_id == id) {
          auto& bundle = *(tasm_mediator.captured_bundles_)[index];
          for (auto [key, value] : bundle.styles) {
            if (target_key == key && target_value.pattern_ == value.pattern_) {
              --count;
            }
          }
        }
        ++index;
      });

  return count == 0;
}

bool FiberElementTest::HasCaptureSignWithStyleKeyAndValue(
    int32_t target_id, CSSPropertyID target_key,
    const tasm::CSSValue& target_value, int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(tasm_mediator.captured_ids_.begin(),
                tasm_mediator.captured_ids_.end(), [&](int32_t id) {
                  if (target_id == id) {
                    auto& bundle = *(tasm_mediator.captured_bundles_)[index];
                    for (auto [key, value] : bundle.styles) {
                      if (target_key == key && target_value == value) {
                        --count;
                      }
                    }
                  }
                  ++index;
                });

  return count == 0;
}

bool FiberElementTest::HasCaptureSignWithResetStyle(int32_t target_id,
                                                    CSSPropertyID target_key,
                                                    int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(tasm_mediator.captured_ids_.begin(),
                tasm_mediator.captured_ids_.end(), [&](int32_t id) {
                  if (target_id == id) {
                    auto& bundle = *(tasm_mediator.captured_bundles_)[index];
                    for (auto [key, value] : bundle.styles) {
                      if (target_key == key) {
                        --count;
                      }
                    }
                  }
                  ++index;
                });

  return count == 0;
}

bool FiberElementTest::HasCaptureSignWithTag(int32_t target_id,
                                             const std::string& target_tag,
                                             int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(tasm_mediator.captured_ids_.begin(),
                tasm_mediator.captured_ids_.end(), [&](int32_t id) {
                  if (target_id == id) {
                    auto& bundle = *(tasm_mediator.captured_bundles_)[index];
                    if (bundle.tag == target_tag) {
                      --count;
                    }
                  }
                  ++index;
                });
  return count == 0;
}

bool FiberElementTest::HasCaptureSignWithInlineParentContainer(
    int32_t target_id, bool is_parent_inline_container, int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(tasm_mediator.captured_ids_.begin(),
                tasm_mediator.captured_ids_.end(), [&](int32_t id) {
                  if (target_id == id) {
                    auto& bundle = *(tasm_mediator.captured_bundles_)[index];
                    if (is_parent_inline_container == bundle.allow_inline) {
                      --count;
                    }
                  }
                  ++index;
                });

  return count == 0;
}

bool FiberElementTest::HasCaptureSignWithFontSize(int32_t target_id,
                                                  double cur_node_font_size,
                                                  double root_node_font_size,
                                                  double font_scale,
                                                  int32_t count) {
  int32_t index = 0;
  std::vector<int32_t> index_ary = {};

  std::for_each(tasm_mediator.captured_ids_.begin(),
                tasm_mediator.captured_ids_.end(), [&](int32_t id) {
                  if (target_id == id) {
                    auto& bundle = *(tasm_mediator.captured_bundles_)[index];
                    if (cur_node_font_size == bundle.cur_node_font_size &&
                        root_node_font_size == bundle.root_node_font_size &&
                        font_scale == bundle.font_scale) {
                      --count;
                    }
                  }
                  ++index;
                });

  return count == 0;
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
