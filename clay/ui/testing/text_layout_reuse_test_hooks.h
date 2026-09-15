// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_TESTING_TEXT_LAYOUT_REUSE_TEST_HOOKS_H_
#define CLAY_UI_TESTING_TEXT_LAYOUT_REUSE_TEST_HOOKS_H_

#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
#include <cstddef>
#include <optional>
#include <unordered_map>

namespace clay {

// Compiled only into TTText unit-test builds. Scope this on the UI thread so
// tests can exercise the real Measure path without changing the trial service.
class ScopedTextLayoutReuseTestHooks {
 public:
  ScopedTextLayoutReuseTestHooks() : previous_(current_) { current_ = this; }
  ~ScopedTextLayoutReuseTestHooks() { current_ = previous_; }
  ScopedTextLayoutReuseTestHooks(const ScopedTextLayoutReuseTestHooks&) =
      delete;
  ScopedTextLayoutReuseTestHooks& operator=(
      const ScopedTextLayoutReuseTestHooks&) = delete;

  void SetEnabled(bool enabled) { enabled_ = enabled; }
  // nullopt leaves activation to the normal platform and rollout checks.
  static std::optional<bool> GetEnabledOverride() {
    return current_ ? std::optional<bool>(current_->enabled_) : std::nullopt;
  }

  static void OnLayout(const void* render) {
    if (current_) {
      ++current_->layouts_[render];
    }
  }
  static void OnTextScan(const void* node) {
    if (current_) {
      ++current_->text_scans_[node];
    }
  }
  size_t Layouts(const void* render) const { return Count(layouts_, render); }
  size_t TextScans(const void* node) const { return Count(text_scans_, node); }

 private:
  using Counts = std::unordered_map<const void*, size_t>;
  static size_t Count(const Counts& counts, const void* key) {
    const auto it = counts.find(key);
    return it == counts.end() ? 0 : it->second;
  }

  inline static thread_local ScopedTextLayoutReuseTestHooks* current_ = nullptr;
  ScopedTextLayoutReuseTestHooks* previous_;
  bool enabled_ = false;
  Counts layouts_;
  Counts text_scans_;
};

}  // namespace clay
#endif  // defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)

#endif  // CLAY_UI_TESTING_TEXT_LAYOUT_REUSE_TEST_HOOKS_H_
