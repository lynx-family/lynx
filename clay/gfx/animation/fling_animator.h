// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_FLING_ANIMATOR_H_
#define CLAY_GFX_ANIMATION_FLING_ANIMATOR_H_

#include <forward_list>
#include <memory>

#include "clay/gfx/animation/dynamic_animator.h"

namespace clay {

/**
 * Fling animation is an animation that continues an initial momentum (most
 * often from gesture velocity) and gradually slows down. The fling animation
 * will come to a stop when the velocity of the animation is below the threshold
 * derived from SetMinimumVisibleChange(float), or when the value of the
 * animation has gone beyond the min or max value defined via SetMinValue(float)
 * or SetMaxValue(float). It is recommended to restrict the fling animation with
 * min and/or max value, such that the animation can end when it goes beyond
 * screen bounds, thus preserving CPU cycles and resources.
 */
class FlingAnimator : public DynamicAnimator {
 public:
  FlingAnimator();

  static void InitParams();

  /**
   * Sets the friction for the fling animation. The greater the friction is, the
   * sooner the animation will slow down. When not set, the friction defaults
   * to 1.
   */
  void SetFriction(float friction_scalar);

  /**
   * Returns the friction being set on the animation.
   */
  float GetFriction();

  /**
   * Returns the estimated distance that the animation will travel.
   */
  float GetDistance();

  void FlingInitialize();

 protected:
  /**
   * Updates the animation state (i.e. value and velocity). Subclasses can
   * override this method to calculate the new value and velocity in their
   * custom way.
   *
   * @param delta_time time elapsed in millisecond since last frame
   * @return whether the animation has finished
   */
  bool UpdateValueAndVelocity(int64_t delta_time) override;

  /**
   * Returns the acceleration at the given value with the given velocity.
   **/
  float GetAcceleration(float value, float velocity) override;

  /**
   * Returns whether the animation has reached equilibrium.
   */
  bool IsAtEquilibrium(float value, float velocity) override;

  /**
   * Updates the default value threshold for the animation based on the property
   * to be animated.
   */
  void SetValueThreshold(float threshold) override;

 private:
  float GetSplineDeceleration(float velocity);
  float GetSplineFlingDuration(float velocity);
  float ComputeDeceleration(float friction);
  float GetSplineFlingDistance(float velocity);

  static constexpr float kDefaultFriction = 0.015f;

  // This multiplier is used to calculate the velocity threshold given a certain
  // value threshold. The idea is that if it takes >= 1 frame to move the value
  // threshold amount, then the velocity is a reasonable threshold.
  static constexpr float kVelocityThresholdMultiplier = 1000.f / 16.f;

  float friction_ = kDefaultFriction;
  float velocity_threshold_;
  float duration_ = 0.f;
  float distance_ = 0.f;
  float physical_coeff_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_FLING_ANIMATOR_H_
