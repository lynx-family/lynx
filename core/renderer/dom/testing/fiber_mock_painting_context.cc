// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/testing/fiber_mock_painting_context.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace lynx {
namespace tasm {
namespace testing {

void FiberMockPaintingContext::ResetFlushFlag() { flush_ = false; }

bool FiberMockPaintingContext::HasFlushed() { return flush_; }

void FiberMockPaintingContext::Flush() {
  queue_->Flush();
  flush_ = true;
}

std::unique_ptr<pub::Value> FiberMockPaintingContext::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  return tasm::TextUtils::GetTextInfo(content, info);
}

std::unordered_map<int, std::string> captured_create_tags_map_;

// TODO(liting.src): remove after painting context refactor.
bool FiberMockPaintingContext::HasEnableUIOperationBatching() { return true; }

void FiberMockPaintingContext::CreatePaintingNode(
    int id, const std::string& tag,
    const std::shared_ptr<PropBundle>& painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {
  EnqueueOperation(
      [this, id, tag_ = tag,
       props_map =
           static_cast<PropBundleMock*>(painting_data.get())->props_]() {
        captured_create_tags_map_.insert(std::make_pair(id, tag_));
        auto node = std::make_unique<MockNode>(id);
        node->props_ = props_map;
        node_map_.insert(std::make_pair(id, std::move(node)));
      });
}
void FiberMockPaintingContext::InsertPaintingNode(int parent, int child,
                                                  int index) {
  EnqueueOperation([this, parent, child, index]() {
    auto* parent_node = node_map_.at(parent).get();
    auto* child_node = node_map_.at(child).get();
    if (index == -1) {
      parent_node->children_.push_back(child_node);
    } else {
      parent_node->children_.insert((parent_node->children_).begin() + index,
                                    child_node);
    }
    child_node->parent_ = parent_node;
  });
}
void FiberMockPaintingContext::RemovePaintingNode(int parent, int child,
                                                  int index, bool is_move) {
  EnqueueOperation([this, parent, child]() {
    auto* parent_node = node_map_.at(parent).get();
    auto* child_node = node_map_.at(child).get();

    auto it_child = std::find(parent_node->children_.begin(),
                              parent_node->children_.end(), child_node);
    if (it_child != parent_node->children_.end()) {
      child_node->parent_ = nullptr;

      parent_node->children_.erase(it_child);
    }
  });
}
void FiberMockPaintingContext::DestroyPaintingNode(int parent, int child,
                                                   int index) {
  EnqueueOperation([this, parent, child]() -> void {
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
  });
}
void FiberMockPaintingContext::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const std::shared_ptr<PropBundle>& painting_data) {
  if (!painting_data) {
    return;
  }
  EnqueueOperation(
      [this, id,
       props_map = static_cast<PropBundleMock*>(painting_data.get())->props_]()
          -> void {
        auto* node = node_map_.at(id).get();
        for (const auto& update : props_map) {
          node->props_[update.first] = update.second;
        }
      });
}
void FiberMockPaintingContext::UpdateLayout(
    int tag, float x, float y, float width, float height, const float* paddings,
    const float* margins, const float* borders, const float* bounds,
    const float* sticky, float max_height, uint32_t node_index) {
  EnqueueOperation([this, x, y, width, height, tag]() -> void {
    if (node_map_.find(tag) == node_map_.end()) {
      return;
    }
    auto* node = node_map_.at(tag).get();
    if (node) {
      node->frame_ = {x, y, width, height};
    }
  });
}

void FiberMockPaintingContext::SetKeyframes(
    std::unique_ptr<PropBundle> keyframes_data) {
  EnqueueOperation(
      [this, props_map =
                 static_cast<PropBundleMock*>(keyframes_data.get())->props_]() {
        for (const auto& item : props_map) {
          keyframes_[item.first] = item.second;
        }
      });
}

int32_t FiberMockPaintingContext::GetTagInfo(const std::string& tag_name) {
  auto it = mock_virtuality_map.find(tag_name);
  if (it != mock_virtuality_map.end()) {
    return it->second;
  } else {
    return 0;
  }
}

bool FiberMockPaintingContext::IsFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  if (func != nullptr) {
    return func(false);
  }
  return false;
}

bool FiberMockPaintingContext::NeedAnimationProps() { return false; }

void FiberMockPaintingContext::EnqueueOperation(shell::UIOperation op) {
  queue_->EnqueueUIOperation(std::move(op));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
