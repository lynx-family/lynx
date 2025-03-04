// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_TESTING_MOCK_PAINTING_CONTEXT_H_
#define CORE_RENDERER_TASM_REACT_TESTING_MOCK_PAINTING_CONTEXT_H_

#define private public

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
#include "core/renderer/utils/test/text_utils_mock.h"

namespace lynx {
namespace tasm {

struct RectF {
  float left_;
  float top_;
  float width_;
  float height_;
};

struct MockNode {
  MockNode(int id) : id_(id) {}
  int id_;
  std::vector<MockNode*> children_;
  MockNode* parent_;
  std::map<std::string, lepus::Value> props_;
  RectF frame_{.0f};
};

class MockPaintingContextPlatformRef : public PaintingCtxPlatformRef {
 public:
  void UpdateNodeReloadPatching(std::vector<int32_t> reload_ids) override {
    reload_ids_ = std::move(reload_ids);
  }
  std::vector<int32_t> reload_ids_;
};

class MockPaintingContext : public PaintingContextPlatformImpl {
 public:
  MockPaintingContext() {
    platform_ref_ = std::make_shared<MockPaintingContextPlatformRef>();
  }
  void ResetFlushFlag() { flush_ = false; }

  bool HasFlushed() { return flush_; }

  virtual void Flush() override { flush_ = true; }
  virtual std::unique_ptr<pub::Value> GetTextInfo(
      const std::string& content, const pub::Value& info) override {
    return tasm::TextUtils::GetTextInfo(content, info);
  }

  // TODO(liting.src): remove after painting context refactor.
  bool HasEnableUIOperationBatching() override { return true; }

 private:
  std::mutex lock_;

  virtual void CreatePaintingNode(
      int id, const std::string& tag,
      const std::shared_ptr<PropBundle>& painting_data, bool flatten,
      bool create_node_async, uint32_t node_index) override {
    std::lock_guard guard(lock_);

    auto node = std::make_unique<MockNode>(id);
    auto* props = painting_data.get();
    if (props) {
      node->props_ = static_cast<PropBundleMock*>(props)->props_;
    }
    node_map_.insert(std::make_pair(id, std::move(node)));
  }
  virtual void InsertPaintingNode(int parent, int child, int index) override {
    std::lock_guard guard(lock_);

    auto* parent_node = node_map_.at(parent).get();
    auto* child_node = node_map_.at(child).get();
    if (index == -1) {
      parent_node->children_.push_back(child_node);
    } else {
      parent_node->children_.insert((parent_node->children_).begin() + index,
                                    child_node);
    }
    child_node->parent_ = parent_node;
  }
  virtual void RemovePaintingNode(int parent, int child, int index,
                                  bool is_move) override {
    std::lock_guard guard(lock_);

    auto* parent_node = node_map_.at(parent).get();
    auto* child_node = node_map_.at(child).get();

    auto it_child = std::find(parent_node->children_.begin(),
                              parent_node->children_.end(), child_node);
    if (it_child != parent_node->children_.end()) {
      child_node->parent_ = nullptr;

      parent_node->children_.erase(it_child);
    }
  }
  virtual void DestroyPaintingNode(int parent, int child, int index) override {
    std::lock_guard guard(lock_);

    auto* child_node = node_map_.at(child).get();
    child_node->parent_ = nullptr;
    if (node_map_.find(parent) != node_map_.end()) {
      auto* parent_node = node_map_.at(parent).get();
      auto it_child = std::find(parent_node->children_.begin(),
                                parent_node->children_.end(), child_node);
      if (it_child != parent_node->children_.end()) {
        parent_node->children_.erase(it_child);
      }
    }

    auto it = node_map_.find(child);
    if (it != node_map_.end()) {
      node_map_.erase(it);
    }
  }
  virtual void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) override {
    std::lock_guard guard(lock_);

    if (!painting_data) {
      return;
    }
    auto* node = node_map_.at(id).get();
    auto* props = painting_data.get();
    for (const auto& update : static_cast<PropBundleMock*>(props)->props_) {
      node->props_[update.first] = update.second;
    }
  }
  virtual void UpdateLayout(int tag, float x, float y, float width,
                            float height, const float* paddings,
                            const float* margins, const float* borders,
                            const float* bounds, const float* sticky,
                            float max_height,
                            uint32_t node_index = 0) override {
    std::lock_guard guard(lock_);

    if (node_map_.find(tag) == node_map_.end()) {
      return;
    }
    auto* node = node_map_.at(tag).get();
    if (node) {
      node->frame_ = {x, y, width, height};
    }
  }

  void SetKeyframes(std::unique_ptr<PropBundle> keyframes_data) override {
    std::lock_guard guard(lock_);

    auto* keyframes = keyframes_data.get();
    for (const auto& item : static_cast<PropBundleMock*>(keyframes)->props_) {
      keyframes_[item.first] = item.second;
    }
  }

  int32_t GetTagInfo(const std::string& tag_name) override {
    std::lock_guard guard(lock_);

    auto it = mock_virtuality_map.find(tag_name);
    if (it != mock_virtuality_map.end()) {
      return it->second;
    } else {
      return 0;
    }
  }

  virtual bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override {
    if (func != nullptr) {
      return func(false);
    }
    return false;
  }

  virtual bool NeedAnimationProps() override { return false; }

 private:
  bool flush_{false};
  std::unordered_map<int, std::unique_ptr<MockNode>> node_map_;
  std::unordered_map<std::string, lepus::Value> keyframes_;
  std::unordered_map<std::string, int32_t> mock_virtuality_map = {
      {"inline-text", LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL},
      {"view", LayoutNodeType::COMMON},
      {"inline-image", LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL},
      {"raw-text", LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL},
      {"text", LayoutNodeType::CUSTOM}};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_TESTING_MOCK_PAINTING_CONTEXT_H_
