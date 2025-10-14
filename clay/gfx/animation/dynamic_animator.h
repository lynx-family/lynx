// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_DYNAMIC_ANIMATOR_H_
#define CLAY_GFX_ANIMATION_DYNAMIC_ANIMATOR_H_

#include <forward_list>
#include <limits>

#include "clay/gfx/animation/animation_handler.h"

namespace clay {

/**
 * This class is the base class of physics-based animations. It manages the
 * animation's lifecycle such as {@link #Start()} and {@link #Cancel()}. This
 * base class also handles the common setup for all the subclass animations.
 * For example, DynamicAnimator supports adding listeners so that the important
 * animation events can be observed through the callbacks. The start conditions
 * for any subclass of DynamicAnimator can be set using SetStartValue(float)
 * and setStartVelocity(float).
 */
class DynamicAnimator : public AnimationHandler::AnimationFrameCallback {
 public:
  /**
   * Implementors of this interface can add themselves as update listeners
   * to an DynamicAnimator instance to receive callbacks on every animation
   * frame, after the current frame's values have been calculated for that
   * DynamicAnimator.
   */
  class AnimationListener {
   public:
    /**
     * Notifies the occurrence of another frame of the animation.
     *
     * @param animation animation that the update listener is added to
     * @param value the current value of the animation
     * @param velocity the current velocity of the animation
     */

    virtual void OnDynamicAnimationUpdate(DynamicAnimator& animation,
                                          float value, float velocity) {}
    /**
     * Notifies the end of an animation. Note that this callback will be
     * invoked not only when an animation reach equilibrium, but also when the
     * animation is canceled.
     *
     * @param animation animation that has ended or was canceled
     * @param canceled whether the animation has been canceled
     * @param value the final value when the animation stopped
     * @param velocity the final velocity when the animation stopped
     */
    virtual void OnDynamicAnimationEnd(DynamicAnimator& animation,
                                       bool canceled, float value,
                                       float velocity) {}

    virtual ~AnimationListener() = default;
  };

  /**
   * The minimum visible change in pixels that can be visible to users.
   */

  static constexpr float kMinVisibleChangePixels = 1.f;

  // Multiplier to the min visible change value for value threshold
  static constexpr float kThresholdMultiplier = 0.75f;

  virtual ~DynamicAnimator();

  /**
   * Sets the start value of the animation. If start value is not set, the
   * animation will get the current value for the view's property, and use that
   * as the start value.
   */

  void SetStartValue(float start_value);

  /**
   * Start velocity of the animation. Default velocity is 0. Unit: change in
   * property per second (e.g. pixels per second, scale/alpha value change per
   * second).
   *
   * Note when using a fixed value as the start velocity (as opposed to getting
   * the velocity through touch events), it is recommended to define such a
   * value in dp/second and convert it to pixel/second based on the density of
   * the screen to achieve a consistent look across different screens.
   */

  void SetStartVelocity(float start_velocity) { velocity_ = start_velocity; }
  float GetCurrentVelocity() const { return velocity_; }

  /**
   * Sets the max value of the animation. Animations will not animate beyond
   * their max value. Whether or not animation will come to an end when max
   * value is reached is dependent on the child animation's implementation.
   */

  void SetMaxValue(float max) {
    // This max value should be checked and handled in the subclass animations,
    // instead of assuming the end of the animations when the max/min value is
    // hit in the base class. The reason is that hitting max/min value may just
    // be a transient state, such as during the spring oscillation.
    max_value_ = max;
  }
  float GetMaxValue() { return max_value_; }

  /**
   * Sets the min value of the animation. Animations will not animate beyond
   * their min value. Whether or not animation will come to an end when min
   * value is reached is dependent on the child animation's implementation.
   */
  void SetMinValue(float min) { min_value_ = min; }
  float GetMinValue() { return min_value_; }

  /**
   * This method sets the minimal change of animation value that is visible to
   * users, which helps determine a reasonable threshold for the animation's
   * termination condition. It is critical to set the minimal visible change
   * for custom properties unless the custom property is in pixels.
   */
  void SetMinimumVisibleChange(float minimum_visible_change);

  /**
   * Returns the minimum change in the animation property that could be visibly
   * different to users.
   *
   * @return minimum change in property value that is visible to users
   */
  float GetMinimumVisibleChange() { return min_visible_change_; }

  /**
   * Adds a listener to the set of listeners that are sent update events through
   * the life of an animation. This method is called on all listeners for every
   * frame of the animation, after the values for the animation have been
   * calculated.
   */
  void AddListener(AnimationListener* listener) {
    listeners_.push_front(listener);
  }

  /**
   * Removes a listener from the set listening to frame updates for this
   * animation.
   */
  void RemoveListener(AnimationListener* listener);

  void RemoveAllListeners();

  // Return the AnimationHandler that will be used to schedule updates for
  // this animator.
  AnimationHandler* GetAnimationHandler() { return animation_handler_; }

  // Sets the animation handler used to schedule updates for this animator
  // or null to use the default handler.
  void SetAnimationHandler(AnimationHandler* animation_handler) {
    animation_handler_ = animation_handler;
  }

  /**
   * Starts an animation. If the animation has already been started, no op. Note
   * that calling
   * Start() will not immediately set the property value to start value
   * of the animation. The property values will be changed at each animation
   * pulse, which happens before the draw pass. As a result, the changes will be
   * reflected in the next frame, the same as if the values were set
   * immediately. This method should only be called on main thread.
   */
  void Start();

  /**
   * Cancels the on-going animation. If the animation hasn't started, no op.
   */
  void Cancel();

  /**
   * Returns whether the animation is currently running.
   */
  bool IsRunning() { return running_; }

  float GetValue() const { return value_; }

  /**
   * Returns the default threshold.
   */
  float GetValueThreshold() {
    return min_visible_change_ * kThresholdMultiplier;
  }

  void SetDensity(float density) { density_ = density; }
  /**
   * This gets call on each frame of the animation. Animation value and
   * velocity are updated in this method based on the new frame time. The
   * property value of the view being animated is then updated. The animation's
   * ending conditions are also checked in this method. Once the animation
   * reaches equilibrium, the animation will come to its end, and end listeners
   * will be notified, if any.
   */
  bool DoAnimationFrame(int64_t frame_time) override;

  // Return time in milliseconds
  int64_t GetLastAnimationTime() const { return last_frame_time_; }

  /**
   * Get the delta value of the last frame.
   */
  float GetDelta() { return delta_; }

 protected:
  /**
   * Updates the animation state (i.e. value and velocity). Subclasses can
   * override this method to calculate the new value and velocity in their
   * custom way.
   *
   * @param delta_time time elapsed in millisecond since last frame
   * @return whether the animation has finished
   */
  virtual bool UpdateValueAndVelocity(int64_t delta_time) = 0;

  /**
   * Returns the acceleration at the given value with the given velocity.
   **/
  virtual float GetAcceleration(float value, float velocity) = 0;

  /**
   * Returns whether the animation has reached equilibrium.
   */
  virtual bool IsAtEquilibrium(float value, float velocity) = 0;

  /**
   * Updates the default value threshold for the animation based on the
   * property to be animated.
   */
  virtual void SetValueThreshold(float threshold) = 0;

 private:
  // This gets called when the animation is started, to finish the setup of the
  // animation before the animation pulsing starts.
  void StartAnimationInternal();

  /**
   * Internal method to reset the animation states when animation is
   * finished/canceled.
   */
  void EndAnimationInternal(bool canceled);

 protected:
  // Internal tracking for velocity.
  float velocity_ = 0;

  // Internal tracking for value.
  float value_ = std::numeric_limits<float>::max();
  float delta_ = 0;

  int64_t start_time_ = 0;
  // Last frame time. Always gets reset to 0 at the end of the animation.
  int64_t last_frame_time_ = 0;

  float start_value_ = 0;
  float final_value_ = 0;
  float density_ = 2.75;

 private:
  void CleanupList();

  bool listener_list_dirty_ = false;

  // Tracks whether start value is set.
  bool start_value_is_set_ = false;

  bool running_ = false;

  // Min and max values that defines the range of the animation values.
  float max_value_ = std::numeric_limits<float>::max();
  float min_value_ = std::numeric_limits<float>::lowest();

  float min_visible_change_ = kMinVisibleChangePixels;

  /**
   * The set of listeners to be sent events through the life of an animation.
   */
  std::forward_list<AnimationListener*> listeners_;

  /**
   * Animation handler used to schedule updates for this animation.
   */
  AnimationHandler* animation_handler_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_DYNAMIC_ANIMATOR_H_
