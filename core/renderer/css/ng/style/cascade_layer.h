// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_CASCADE_LAYER_H_
#define CORE_RENDERER_CSS_NG_STYLE_CASCADE_LAYER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace css {

class CascadeLayer {
 public:
  static constexpr uint16_t kImplicitOuterLayerOrder = 0xFFFF;

  CascadeLayer() = default;
  explicit CascadeLayer(std::string name) : name_(std::move(name)) {}

  const std::string& GetName() const { return name_; }

  const std::vector<std::unique_ptr<CascadeLayer>>& GetDirectSubLayers() const {
    return sub_layers_;
  }

  uint16_t GetOrder() const { return order_; }
  void SetOrder(uint16_t order) { order_ = order; }

  // Navigate a dot-separated layer path (represented as a vector of segments),
  // creating intermediate nodes as needed. Returns the target layer node.
  CascadeLayer* GetOrAddSubLayer(
      const std::vector<std::string>& name_segments) {
    CascadeLayer* current = this;
    for (const auto& segment : name_segments) {
      current = current->GetOrAddDirectSubLayer(segment);
    }
    return current;
  }

  // Find an existing direct child with the given name, or create one.
  // Anonymous layers (empty name) always create a new child.
  CascadeLayer* GetOrAddDirectSubLayer(const std::string& name) {
    if (!name.empty()) {
      for (auto& child : sub_layers_) {
        if (child->name_ == name) {
          return child.get();
        }
      }
    }
    sub_layers_.push_back(std::make_unique<CascadeLayer>(name));
    return sub_layers_.back().get();
  }

 private:
  std::string name_;
  std::vector<std::unique_ptr<CascadeLayer>> sub_layers_;
  uint16_t order_ = kImplicitOuterLayerOrder;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_CASCADE_LAYER_H_
