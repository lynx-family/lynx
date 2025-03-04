// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_TASM_TESTING_MOCK_ATTRIBUTE_HOLDER_H_
#define CORE_RENDERER_TASM_TESTING_MOCK_ATTRIBUTE_HOLDER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {

struct MockAttributeHolder : public tasm::AttributeHolder {
  explicit MockAttributeHolder(const std::string& tag)
      : tasm::AttributeHolder() {
    set_tag(tag);
  }

  void SetParent(AttributeHolder* parent) { parent_ = parent; }

  void AddChild(std::unique_ptr<MockAttributeHolder> child, int index) {
    child->SetParent(this);
    children_.insert(children_.begin() + index, std::move(child));
  }

  void AddChild(std::unique_ptr<MockAttributeHolder> child) {
    child->SetParent(this);
    children_.emplace_back(std::move(child));
  }

  std::unique_ptr<MockAttributeHolder> RemoveChild(MockAttributeHolder* child) {
    child->SetParent(nullptr);
    auto it = find_if(children_.begin(), children_.end(),
                      [child](std::unique_ptr<MockAttributeHolder>& item) {
                        return item.get() == child;
                      });
    if (it == children_.end()) {
      return std::unique_ptr<MockAttributeHolder>(nullptr);
    } else {
      auto deleted_child = std::move(*it);
      children_.erase(it);
      return deleted_child;
    }
  }

  const std::vector<std::unique_ptr<MockAttributeHolder>>& children() const {
    return children_;
  }

  AttributeHolder* Sibling(int offset) const {
    if (!HolderParent()) return nullptr;
    const auto& siblings =
        static_cast<const MockAttributeHolder*>(HolderParent())->children();
    auto iter = std::find_if(siblings.begin(), siblings.end(),
                             [this](auto& ptr) { return ptr.get() == this; });
    auto dist = std::distance(siblings.begin(), iter) + offset;
    if (dist < 0 || dist >= static_cast<long>(siblings.size())) {
      return nullptr;
    }
    return siblings[dist].get();
  }

  AttributeHolder* NextSibling() const override { return Sibling(1); }

  AttributeHolder* PreviousSibling() const override { return Sibling(-1); }

  AttributeHolder* HolderParent() const override { return parent_; }

 private:
  AttributeHolder* parent_ = nullptr;
  std::vector<std::unique_ptr<MockAttributeHolder>> children_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_TESTING_MOCK_ATTRIBUTE_HOLDER_H_
