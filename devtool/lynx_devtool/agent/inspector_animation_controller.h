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
// semantics. Lifecycle notifications and CDP commands are handled on TASM.
// Animation events are posted to DevTool through the mediator.
class InspectorAnimationController {
 public:
  explicit InspectorAnimationController(
      std::weak_ptr<LynxDevToolMediator> devtool_mediator);

  void OnAnimationCreated(int64_t id,
                          std::weak_ptr<animation::Animation>&& animation);
  void OnAnimationStarted(const Json::Value& snapshot);
  void OnAnimationUpdated(const Json::Value& snapshot);
  void OnAnimationCanceled(int64_t id);

  // Builds the CDP Animation JSON object for |animation| (id, name, play
  // state, timing, source AnimationEffect with keyframesRule, backendNodeId,
  // type). Observers call this on the animation callback thread before posting
  // the notification to TASM.
  static Json::Value BuildAnimationSnapshot(animation::Animation* animation);

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
  void SendEvent(const std::string& method, const Json::Value& params) const;

  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  // Maps animation IDs to weak Animation references because an animation may
  // be freed before the task posted to TASM runs. Accessed only on TASM.
  std::unordered_map<int64_t, std::weak_ptr<animation::Animation>>
      animation_registry_;
  // Keep released IDs until cancellation or page teardown so delayed Created
  // notifications cannot register them again.
  std::unordered_set<int64_t> released_animation_ids_;
  bool enabled_{false};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_CONTROLLER_H_
