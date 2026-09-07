// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_MOCK_ANIMATION_TARGET_H_
#define CORE_LIST_TESTING_MOCK_ANIMATION_TARGET_H_

#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/list/animation/animation_target.h"

namespace lynx {
namespace list {

class MockAnimationTarget final
    : public AnimationTarget,
      public fml::EnableWeakFromThis<MockAnimationTarget> {
 public:
  struct OpacityUpdate {
    float opacity{0.f};
    bool flush_immediately{false};
  };

  struct PositionUpdate {
    float left{0.f};
    float top{0.f};
    bool flush_immediately{false};
  };

  explicit MockAnimationTarget(std::string key) : key_(std::move(key)) {}
  ~MockAnimationTarget() override = default;

  void SetAnimationLayout(int index, float left, float top, float width,
                          float height) {
    index_ = index;
    left_ = left;
    top_ = top;
    width_ = width;
    height_ = height;
  }

  WeakAnimationTarget GetWeakAnimationTarget() const override {
    return WeakAnimationTarget(
        fml::EnableWeakFromThis<MockAnimationTarget>::WeakFromThis());
  }

  const std::string& GetAnimationKey() const override { return key_; }
  int GetAnimationIndex() const override { return index_; }
  float GetAnimationLeft() const override { return left_; }
  float GetAnimationTop() const override { return top_; }
  float GetAnimationWidth() const override { return width_; }
  float GetAnimationHeight() const override { return height_; }

  void PrepareForAnimation(ItemAnimationType animation_type) override {
    prepared_animation_types_.emplace_back(animation_type);
  }
  void FinishAnimation() override { ++finish_animation_count_; }
  void UpdateAnimationOpacity(float opacity, bool flush_immediately) override {
    opacity_updates_.emplace_back(OpacityUpdate{
        .opacity = opacity, .flush_immediately = flush_immediately});
  }
  void UpdateAnimationPosition(float left, float top,
                               bool flush_immediately) override {
    position_updates_.emplace_back(PositionUpdate{
        .left = left,
        .top = top,
        .flush_immediately = flush_immediately,
    });
  }

  const std::vector<ItemAnimationType>& prepared_animation_types() const {
    return prepared_animation_types_;
  }
  int finish_animation_count() const { return finish_animation_count_; }
  const std::vector<OpacityUpdate>& opacity_updates() const {
    return opacity_updates_;
  }
  const std::vector<PositionUpdate>& position_updates() const {
    return position_updates_;
  }
  void ClearAnimationUpdates() {
    opacity_updates_.clear();
    position_updates_.clear();
  }

 private:
  std::string key_;
  int index_{0};
  float left_{0.f};
  float top_{0.f};
  float width_{0.f};
  float height_{0.f};
  std::vector<ItemAnimationType> prepared_animation_types_;
  int finish_animation_count_{0};
  std::vector<OpacityUpdate> opacity_updates_;
  std::vector<PositionUpdate> position_updates_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_MOCK_ANIMATION_TARGET_H_
