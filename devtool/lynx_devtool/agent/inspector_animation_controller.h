// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_CONTROLLER_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_CONTROLLER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace animation {
class Animation;
}  // namespace animation
namespace devtool {

class CDPResponder;
class LynxDevToolMediator;

// Owns Animation-domain state, CDP payload construction, and command
// semantics. Every method is called on the TASM thread. Animation events are
// posted to the DevTool thread through the mediator when it is still alive.
class InspectorAnimationController {
 public:
  explicit InspectorAnimationController(
      std::weak_ptr<LynxDevToolMediator> devtool_mediator);

  void OnAnimationCreated(animation::Animation* animation);
  void OnAnimationStarted(animation::Animation* animation);
  void OnAnimationUpdated(animation::Animation* animation);
  void OnAnimationCanceled(animation::Animation* animation);

  void Enable(const std::shared_ptr<CDPResponder>& responder,
              const Json::Value& params);
  void Disable(const std::shared_ptr<CDPResponder>& responder,
               const Json::Value& params);
  void GetCurrentTime(const std::shared_ptr<CDPResponder>& responder,
                      const Json::Value& params);
  void SeekAnimations(const std::shared_ptr<CDPResponder>& responder,
                      const Json::Value& params);
  void SetPaused(const std::shared_ptr<CDPResponder>& responder,
                 const Json::Value& params);
  void ReleaseAnimations(const std::shared_ptr<CDPResponder>& responder,
                         const Json::Value& params);

  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) { enabled_ = enabled; }
  void Clear();

 private:
  // Builds the CDP Animation JSON object for |animation| (id, name, play
  // state, timing, source AnimationEffect with keyframesRule, backendNodeId,
  // type).
  Json::Value BuildAnimationSnapshot(animation::Animation* animation);
  void SendEvent(const std::string& method, const Json::Value& params) const;

  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  // Maps stable Animation.id values to live, non-owning Animation pointers.
  // OnAnimationCanceled runs synchronously from Animation destruction and
  // removes the entry before its animation can be freed.
  std::unordered_map<int64_t, animation::Animation*> animation_registry_;
  // Animation IDs are never reused. Keep releases until cancellation or page
  // teardown so a late started callback cannot make an animation controllable
  // again.
  std::unordered_set<int64_t> released_animation_ids_;
  bool enabled_{false};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_CONTROLLER_H_
