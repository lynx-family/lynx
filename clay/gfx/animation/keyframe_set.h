// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_KEYFRAME_SET_H_
#define CLAY_GFX_ANIMATION_KEYFRAME_SET_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/gfx/animation/animator_listener_adapter.h"
#include "clay/gfx/animation/keyframe.h"
#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class KeyframesManager;

class KeyframeSet : public AnimatorListenerAdapter {
 public:
  KeyframeSet(const KeyframeSet&) = delete;
  KeyframeSet& operator=(const KeyframeSet&) = delete;

  virtual ~KeyframeSet();

  ClayAnimationPropertyType Type() const { return property_type_; }

  // Create an instance of the keyframes and associate a manager to it. This
  // will resolve the percentage size (if any)
  virtual std::unique_ptr<KeyframeSet> Clone(
      KeyframesManager* manager) const = 0;

#ifndef NDEBUG
  virtual std::string ToString() const = 0;
#endif

  // Return true if there is any percentage value. Currently only
  // `RawTransformKeyframeSet` may return true
  virtual bool HasPercentageValues() const { return false; }

  void SetKeyframesManager(KeyframesManager* keyframes_manager);
  KeyframesManager* GetKeyframesManager() const { return keyframes_manager_; }

 protected:
  explicit KeyframeSet(ClayAnimationPropertyType property_type);

 private:
  // The name of the property associated with keyframes_.
  ClayAnimationPropertyType property_type_;
  KeyframesManager* keyframes_manager_ = nullptr;
};

class FloatKeyframeSet : public KeyframeSet {
 public:
  FloatKeyframeSet(const FloatKeyframeSet&) = delete;
  FloatKeyframeSet& operator=(const FloatKeyframeSet&) = delete;

  static std::unique_ptr<FloatKeyframeSet> Create(
      ClayAnimationPropertyType property_type);
  ~FloatKeyframeSet() override;

  std::unique_ptr<KeyframeSet> Clone(KeyframesManager* manager) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  // AnimatorListenerAdapter overrides
  void OnAnimationStart(Animator& animation) override;
  void OnAnimationUpdate(ValueAnimator& animation) override;
  void OnAnimationRemove(Animator& animation) override;
  /**
   * Gets the animated value, given the elapsed fraction of the animation
   * (interpolated by the animation's interpolator) and the evaluator used
   * to calculate in-between values.
   *
   * @param fraction The elapsed fraction of the animation
   * @return The animated value.
   */
  float GetValue(float fraction) const;

  void AddKeyframe(std::unique_ptr<FloatKeyframe> keyframe);

  using Keyframes = std::vector<std::unique_ptr<FloatKeyframe>>;

 private:
  FRIEND_TEST(KeyFrameTest, DefaultStartAndEnd);
  FRIEND_TEST(KeyFrameTest, UpdateAnimation);

  explicit FloatKeyframeSet(ClayAnimationPropertyType property_type);

  // Always sorted in order of increasing time. No two keyframes have the
  // same time.
  Keyframes keyframes_;

  float original_value_ = 0.f;
};

class ColorKeyframeSet : public KeyframeSet {
 public:
  ColorKeyframeSet(const ColorKeyframeSet&) = delete;
  ColorKeyframeSet& operator=(const ColorKeyframeSet&) = delete;

  static std::unique_ptr<ColorKeyframeSet> Create(
      ClayAnimationPropertyType property_type);
  ~ColorKeyframeSet() override;

  std::unique_ptr<KeyframeSet> Clone(KeyframesManager* manager) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  // AnimatorListenerAdapter overrides
  void OnAnimationStart(Animator& animation) override;
  void OnAnimationUpdate(ValueAnimator& animation) override;
  void OnAnimationRemove(Animator& animation) override;

  /**
   * Gets the animated value, given the elapsed fraction of the animation
   * (interpolated by the animation's interpolator) and the evaluator used
   * to calculate in-between values.
   *
   * @param fraction The elapsed fraction of the animation
   * @return The animated value.
   */
  Color GetValue(float fraction) const;

  void AddKeyframe(std::unique_ptr<ColorKeyframe> keyframe);

  using Keyframes = std::vector<std::unique_ptr<ColorKeyframe>>;

 private:
  explicit ColorKeyframeSet(ClayAnimationPropertyType property_type);

  // Always sorted in order of increasing time. No two keyframes have the
  // same time.
  Keyframes keyframes_;

  Color original_value_ = Color();
};

// This class is only used to store the raw data of transform operations
// (without resolving percentage size) in the `PageView::keyframes_data_`. The
// real animations is executed on the `TransformKeyframeSet` objects which can
// be created by calling the `Clone` method of this class.
class RawTransformKeyframeSet : public KeyframeSet {
 public:
  static std::unique_ptr<RawTransformKeyframeSet> Create();

  std::unique_ptr<KeyframeSet> Clone(KeyframesManager* manager) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  bool HasPercentageValues() const override { return has_percentage_values_; }

  void AddKeyframe(std::unique_ptr<RawTransformKeyframe> keyframe);

  using Keyframes = std::vector<std::unique_ptr<RawTransformKeyframe>>;

 private:
  RawTransformKeyframeSet();

  Keyframes keyframes_;
  bool has_percentage_values_ = false;
};

class TransformKeyframeSet : public KeyframeSet {
 public:
  TransformKeyframeSet(const TransformKeyframeSet&) = delete;
  TransformKeyframeSet& operator=(const TransformKeyframeSet&) = delete;

  static std::unique_ptr<TransformKeyframeSet> Create(
      ClayAnimationPropertyType property_type);
  ~TransformKeyframeSet() override;

  std::unique_ptr<KeyframeSet> Clone(KeyframesManager* manager) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  // AnimatorListenerAdapter overrides
  void OnAnimationStart(Animator& animation) override;
  void OnAnimationUpdate(ValueAnimator& animation) override;
  void OnAnimationRemove(Animator& animation) override;

  /**
   * Gets the animated value, given the elapsed fraction of the animation
   * (interpolated by the animation's interpolator) and the evaluator used
   * to calculate in-between values.
   *
   * @param fraction The elapsed fraction of the animation
   * @return The animated value.
   */
  TransformOperations GetValue(float fraction) const;

  void AddKeyframe(std::unique_ptr<TransformKeyframe> keyframe);

  using Keyframes = std::vector<std::unique_ptr<TransformKeyframe>>;

 private:
  explicit TransformKeyframeSet(ClayAnimationPropertyType property_type);

  // Always sorted in order of increasing time. No two keyframes have the
  // same time.
  Keyframes keyframes_;

  TransformOperations original_value_ = TransformOperations();
};

class FilterKeyframeSet : public KeyframeSet {
 public:
  FilterKeyframeSet(const FilterKeyframeSet&) = delete;
  FilterKeyframeSet& operator=(const FilterKeyframeSet&) = delete;

  static std::unique_ptr<FilterKeyframeSet> Create();
  ~FilterKeyframeSet() override;

  std::unique_ptr<KeyframeSet> Clone(KeyframesManager* manager) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  // AnimatorListenerAdapter overrides
  void OnAnimationStart(Animator& animation) override;
  void OnAnimationUpdate(ValueAnimator& animation) override;
  void OnAnimationRemove(Animator& animation) override;

  FilterOperations GetValue(float fraction) const;

  void AddKeyframe(std::unique_ptr<FilterKeyframe> keyframe);

  using Keyframes = std::vector<std::unique_ptr<FilterKeyframe>>;

 private:
  explicit FilterKeyframeSet();

  // Always sorted in order of increasing time. No two keyframes have the
  // same time.
  Keyframes keyframes_;

  FilterOperations original_value_ = FilterOperations();
};

class BoxShadowKeyframeSet : public KeyframeSet {
 public:
  BoxShadowKeyframeSet(const BoxShadowKeyframeSet&) = delete;
  BoxShadowKeyframeSet& operator=(const BoxShadowKeyframeSet&) = delete;

  static std::unique_ptr<BoxShadowKeyframeSet> Create();
  ~BoxShadowKeyframeSet() override;

  std::unique_ptr<KeyframeSet> Clone(KeyframesManager* manager) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  // AnimatorListenerAdapter overrides
  void OnAnimationStart(Animator& animation) override;
  void OnAnimationUpdate(ValueAnimator& animation) override;
  void OnAnimationRemove(Animator& animation) override;

  BoxShadowOperations GetValue(float fraction) const;

  void AddKeyframe(std::unique_ptr<BoxShadowKeyframe> keyframe);

  using Keyframes = std::vector<std::unique_ptr<BoxShadowKeyframe>>;

 private:
  explicit BoxShadowKeyframeSet();

  // Always sorted in order of increasing time. No two keyframes have the
  // same time.
  Keyframes keyframes_;

  BoxShadowOperations original_value_ = BoxShadowOperations();
};

using KeyframesMap =
    std::unordered_map<ClayAnimationPropertyType, std::unique_ptr<KeyframeSet>>;
using KeyframesMapData = std::unordered_map<std::string, KeyframesMap>;

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_KEYFRAME_SET_H_
