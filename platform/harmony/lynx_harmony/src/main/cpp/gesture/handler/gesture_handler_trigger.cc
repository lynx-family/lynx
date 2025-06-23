// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/gesture_handler_trigger.h"

#include <cfloat>

#include "core/renderer/events/gesture.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/detector/gesture_detector_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_handler_delegate.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {

GestureHandlerTrigger::GestureHandlerTrigger(
    LynxContext* context, std::shared_ptr<GestureDetectorManager> manager)
    : context_(context),
      last_fling_target_id_(0),
      velocity_x_(0),
      velocity_y_(0),
      gesture_detector_manager_(manager),
      fling_scroller_(std::make_unique<FlingScroller>()) {}

void GestureHandlerTrigger::InitCurrentWinnerWhenDown(
    std::weak_ptr<GestureArenaMember> member) {
  winner_ = member;
  UpdateLastWinner(winner_);
  UpdateSimultaneousWinner(winner_);
  ResetGestureHandlerAndSimultaneous(winner_);
}

void GestureHandlerTrigger::ResolveTouchEvent(
    const ArkUI_UIInputEvent* event,
    std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
    std::shared_ptr<TouchEvent> lynx_touch_event,
    std::vector<std::weak_ptr<GestureArenaMember>>& bubble_chain_candidates) {
  auto touch_action = OH_ArkUI_UIInputEvent_GetAction(event);
  float page_point[2] = {OH_ArkUI_PointerEvent_GetXByIndex(event, 0),
                         OH_ArkUI_PointerEvent_GetYByIndex(event, 0)};

  if (touch_action == UI_TOUCH_EVENT_ACTION_DOWN) {
    ResetCandidatesGestures(compete_chain_candidates);
    StopFlingByLastFlingMember(lynx_touch_event, compete_chain_candidates,
                               bubble_chain_candidates, event);
    DispatchMotionEventWithSimultaneousAndReCompete(
        winner_, page_point[0], page_point[1], lynx_touch_event,
        compete_chain_candidates, event);
    FindNextWinnerInBegin(lynx_touch_event, compete_chain_candidates,
                          page_point[0], page_point[1], event);

  } else if (touch_action == UI_TOUCH_EVENT_ACTION_MOVE) {
    winner_ = ReCompeteByGestures(compete_chain_candidates, winner_);
    if (winner_.lock() == last_winner_.lock()) {
      DispatchMotionEventWithSimultaneous(winner_, page_point[0], page_point[1],
                                          lynx_touch_event, event);
    }
    FindNextWinnerInBegin(lynx_touch_event, compete_chain_candidates,
                          page_point[0], page_point[1], event);
  } else if (touch_action == UI_TOUCH_EVENT_ACTION_UP ||
             touch_action == UI_TOUCH_EVENT_ACTION_CANCEL) {
    DispatchMotionEventWithSimultaneousAndReCompete(
        winner_, page_point[0], page_point[1], nullptr,
        compete_chain_candidates, event);
    if (winner_.lock() &&
        (abs(velocity_x_) > GestureConstants::FLING_SPEED_THRESHOLD ||
         abs(velocity_y_) > GestureConstants::FLING_SPEED_THRESHOLD)) {
      compete_chain_candidates_ = compete_chain_candidates;
      fling_scroller_->Start(
          velocity_x_, velocity_y_,
          std::bind(&GestureHandlerTrigger::FlingCallback, this,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3));
    } else {
      DispatchMotionEventWithSimultaneousAndReCompete(
          winner_, FLT_MIN, FLT_MIN, nullptr, compete_chain_candidates,
          nullptr);
    }
  }
}

void GestureHandlerTrigger::SetVelocity(float velocity_x, float velocity_y) {
  velocity_x_ = velocity_x;
  velocity_y_ = velocity_y;
}

void GestureHandlerTrigger::FlingCallback(int status, float delta_x,
                                          float delta_y) {
  delta_x = delta_x / context_->DevicePixelRatio();
  delta_y = delta_y / context_->DevicePixelRatio();

  winner_ = ReCompeteByGestures(compete_chain_candidates_, winner_);
  FindNextWinnerInBegin(nullptr, compete_chain_candidates_, 0, 0, nullptr);
  if (winner_.lock()) {
    last_fling_target_id_ = winner_.lock()->GestureArenaMemberId();
    DispatchMotionEventWithSimultaneous(winner_, delta_x, delta_y, nullptr,
                                        nullptr);
    if (fling_scroller_->IsIdle()) {
      DispatchMotionEventWithSimultaneousAndReCompete(
          winner_, FLT_MIN, FLT_MIN, nullptr, compete_chain_candidates_,
          nullptr);
    }
  } else {
    last_fling_target_id_ = 0;
    if (!fling_scroller_->IsIdle()) {
      fling_scroller_->Stop();
    }
  }
}

void GestureHandlerTrigger::HandleGestureDetectorState(
    std::weak_ptr<GestureArenaMember> member, int gesture_id, int state) {
  if (!member.lock()) {
    return;
  }
  auto handler = GetGestureHandlerById(member, gesture_id);
  if (state ==
      static_cast<int>(GestureHandlerDelegate::LynxGestureState::FAIL)) {
    if (handler) {
      handler->Fail();
    }
  } else if (state ==
             static_cast<int>(GestureHandlerDelegate::LynxGestureState::END)) {
    if (handler) {
      handler->End();
    }
  }
}

void GestureHandlerTrigger::DispatchBubbleTouchEvent(
    const std::string& type, std::shared_ptr<TouchEvent> touch_event,
    std::vector<std::weak_ptr<GestureArenaMember>>& bubble_candidate,
    std::weak_ptr<GestureArenaMember> winner) {
  if (!winner.lock()) {
    return;
  }

  if (type != TouchEvent::START && type != TouchEvent::MOVE &&
      type != TouchEvent::CANCEL && type != TouchEvent::UP) {
    return;
  }

  for (const auto& member : bubble_candidate) {
    auto gesture_handler_map = member.lock()->GetGestureHandlers();
    if (gesture_handler_map.empty()) {
      continue;
    }
    for (const auto& handler_entry : gesture_handler_map) {
      auto& handler = handler_entry.second;
      if (type == TouchEvent::START) {
        handler->OnTouchesDown(touch_event);
      } else if (type == TouchEvent::MOVE) {
        handler->OnTouchesMove(touch_event);
      } else if (type == TouchEvent::UP) {
        handler->OnTouchesUp(touch_event);
      } else if (type == TouchEvent::CANCEL) {
        handler->OnTouchesCancel(touch_event);
      }
    }
  }
}

void GestureHandlerTrigger::Destroy() {
  context_ = nullptr;
  if (!simultaneous_winners_.empty()) {
    simultaneous_winners_.clear();
  }
}

void GestureHandlerTrigger::ResetCandidatesGestures(
    std::vector<std::weak_ptr<GestureArenaMember>>& members) {
  for (const auto& member : members) {
    ResetGestureHandlerAndSimultaneous(member);
  }
  duplicated_member_ = std::weak_ptr<GestureArenaMember>();
}

void GestureHandlerTrigger::FailOthersMembersInRaceRelation(
    std::weak_ptr<GestureArenaMember> member, int current_gesture_id,
    std::unordered_set<int>& simultaneous_gesture_ids) {
  if (!member.lock()) {
    return;
  }
  auto gesture_handler_map = member.lock()->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return;
  }
  for (const auto& handler_entry : gesture_handler_map) {
    auto& handler = handler_entry.second;
    if (handler->GetGestureDetector()->gesture_id() != current_gesture_id &&
        simultaneous_gesture_ids.find(
            handler->GetGestureDetector()->gesture_id()) ==
            simultaneous_gesture_ids.end()) {
      handler->Fail();
    }
  }
}

void GestureHandlerTrigger::StopFlingByLastFlingMember(
    std::shared_ptr<TouchEvent> lynx_touch_event,
    std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
    std::vector<std::weak_ptr<GestureArenaMember>>& bubble_candidates,
    const ArkUI_UIInputEvent* touch_event) {
  if (bubble_candidates.empty()) {
    return;
  }
  for (const auto& member : bubble_candidates) {
    if ((winner_.lock() &&
         last_fling_target_id_ == member.lock()->GestureArenaMemberId()) ||
        last_fling_target_id_ == 0) {
      last_fling_target_id_ = 0;
      if (!fling_scroller_->IsIdle()) {
        DispatchMotionEventWithSimultaneousAndReCompete(
            winner_, 0, 0, lynx_touch_event, compete_chain_candidates,
            touch_event);
        fling_scroller_->Stop();
        if (context_) {
          context_->OnGestureRecognizedWithSign(member.lock()->Sign());
        }
      }
      break;
    }
  }
}

void GestureHandlerTrigger::FindNextWinnerInBegin(
    std::shared_ptr<TouchEvent> lynx_touch_event,
    std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
    float x, float y, const ArkUI_UIInputEvent* event) {
  for (int i = 0; i < compete_chain_candidates.size(); ++i) {
    if (winner_.lock() == last_winner_.lock() || !winner_.lock()) {
      return;
    }
    UpdateLastWinner(winner_);
    UpdateSimultaneousWinner(winner_);
    DispatchMotionEventWithSimultaneousAndReCompete(
        winner_, x, y, lynx_touch_event, compete_chain_candidates, event);
  }
  DispatchMotionEventWithSimultaneousAndReCompete(
      last_winner_, x, y, lynx_touch_event, compete_chain_candidates, event);
}

void GestureHandlerTrigger::UpdateSimultaneousWinner(
    std::weak_ptr<GestureArenaMember> winner) {
  auto result = gesture_detector_manager_->HandleSimultaneousWinner(winner);
  simultaneous_winners_ = result.first;
  simultaneous_gesture_ids_ = result.second;
}

void GestureHandlerTrigger::UpdateLastWinner(
    std::weak_ptr<GestureArenaMember> winner) {
  if (last_winner_.lock() != winner.lock()) {
    last_winner_ = winner;
  }
}

std::weak_ptr<GestureArenaMember> GestureHandlerTrigger::ReCompeteByGestures(
    std::vector<std::weak_ptr<GestureArenaMember>>& competitor_chain_candidates,
    std::weak_ptr<GestureArenaMember> current) {
  if ((!current.lock() && !last_winner_.lock()) ||
      !competitor_chain_candidates.size()) {
    return std::weak_ptr<GestureArenaMember>();
  }

  bool need_reCompete_last_winner = false;
  if (!current.lock() && last_winner_.lock()) {
    need_reCompete_last_winner = true;
    current = last_winner_;
    ResetGestureHandlerAndSimultaneous(last_winner_);
  }

  int state_current = GetCurrentMemberState(current);
  if (state_current <= GestureConstants::LYNX_STATE_ACTIVE) {
    return current;
  } else if (state_current == GestureConstants::LYNX_STATE_END) {
    return std::weak_ptr<GestureArenaMember>();
  }

  if (need_reCompete_last_winner) {
    return std::weak_ptr<GestureArenaMember>();
  }

  auto it = std::find_if(
      competitor_chain_candidates.begin(), competitor_chain_candidates.end(),
      [&current](const std::weak_ptr<GestureArenaMember>& elem) {
        auto current_lock = current.lock();
        auto elem_lock = elem.lock();
        if (!current_lock || !elem_lock) {
          return false;
        }
        return current_lock->GestureArenaMemberId() ==
               elem_lock->GestureArenaMemberId();
      });

  if (it == competitor_chain_candidates.end()) {
    return std::weak_ptr<GestureArenaMember>();
  }

  auto last_it =
      std::find_if(competitor_chain_candidates.rbegin(),
                   competitor_chain_candidates.rend(),
                   [&current](const std::weak_ptr<GestureArenaMember>& elem) {
                     auto current_lock = current.lock();
                     auto elem_lock = elem.lock();
                     return current_lock && elem_lock &&
                            current_lock->GestureArenaMemberId() ==
                                elem_lock->GestureArenaMemberId();
                   })
          .base() -
      1;

  if (it != last_it) {
    if (duplicated_member_.lock()) {
      it = last_it - 1;
      ResetGestureHandlerAndSimultaneous(duplicated_member_);
    } else {
      duplicated_member_ = *it;
    }
  }
  if (it == competitor_chain_candidates.end() ||
      it < competitor_chain_candidates.begin() ||
      it >= competitor_chain_candidates.end()) {
    return std::weak_ptr<GestureArenaMember>();
  }

  int current_member_id = (*it).lock()->GestureArenaMemberId();

  for (auto next_it = it + 1; next_it != competitor_chain_candidates.end();
       ++next_it) {
    auto node = *next_it;
    if (node.lock()->GestureArenaMemberId() == current_member_id) {
      continue;
    }
    if (duplicated_member_.lock() == node.lock()) {
      duplicated_member_.reset();
    } else {
      ResetGestureHandlerAndSimultaneous(node);
    }

    int state = GetCurrentMemberState(node);
    if (state <= GestureConstants::LYNX_STATE_ACTIVE) {
      return node;
    } else if (state == GestureConstants::LYNX_STATE_END) {
      return std::weak_ptr<GestureArenaMember>();
    }
  }

  for (auto pre_it = competitor_chain_candidates.begin(); pre_it != it;
       ++pre_it) {
    auto node = *pre_it;
    if (node.lock()->GestureArenaMemberId() == current_member_id) {
      continue;
    }
    if (duplicated_member_.lock() == node.lock()) {
      duplicated_member_.reset();
    } else {
      ResetGestureHandlerAndSimultaneous(node);
    }

    int state = GetCurrentMemberState(node);
    if (state <= GestureConstants::LYNX_STATE_ACTIVE) {
      return node;
    } else if (state == GestureConstants::LYNX_STATE_END) {
      return std::weak_ptr<GestureArenaMember>();
    }
  }

  return std::weak_ptr<GestureArenaMember>();
}

void GestureHandlerTrigger::DispatchMotionEventOnCurrentWinner(
    const ArkUI_UIInputEvent* event, std::weak_ptr<GestureArenaMember> member,
    std::shared_ptr<TouchEvent> lynx_touch_event, float delta_x,
    float delta_y) {
  if (!member.lock()) {
    return;
  }
  auto gesture_handler_map = member.lock()->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return;
  }

  for (auto& handler_pair : gesture_handler_map) {
    handler_pair.second->HandleMotionEvent(event, lynx_touch_event, delta_x,
                                           delta_y);
  }
}

int GestureHandlerTrigger::GetCurrentMemberState(
    std::weak_ptr<GestureArenaMember> node) {
  if (!node.lock()) {
    return GestureConstants::LYNX_STATE_FAIL;
  }

  auto gesture_handler_map = node.lock()->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return GestureConstants::LYNX_STATE_FAIL;
  }

  int min_status = -1;
  for (auto& handler_pair : gesture_handler_map) {
    auto& handler = handler_pair.second;
    if (handler->IsEnd()) {
      ResetGestureHandlerAndSimultaneous(node);
      last_winner_.lock() = nullptr;
      return GestureConstants::LYNX_STATE_END;
    }

    if (handler->IsActive()) {
      FailOthersMembersInRaceRelation(
          node, handler->GetGestureDetector()->gesture_id(),
          simultaneous_gesture_ids_);
      return GestureConstants::LYNX_STATE_ACTIVE;
    }

    int status = handler->GetGestureStatus();
    if (min_status < 0 || status < min_status) {
      min_status = status;
    }
  }

  return min_status;
}

void GestureHandlerTrigger::ResetGestureHandlerAndSimultaneous(
    std::weak_ptr<GestureArenaMember> member) {
  ResetGestureHandler(member);

  for (const auto& arena_member : simultaneous_winners_) {
    ResetGestureHandler(arena_member);
  }
}

void GestureHandlerTrigger::ResetGestureHandler(
    std::weak_ptr<GestureArenaMember> member) {
  if (!member.lock()) {
    return;
  }

  auto gesture_handler_map = member.lock()->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return;
  }

  for (auto& handler_pair : gesture_handler_map) {
    handler_pair.second->Reset();
  }
}

void GestureHandlerTrigger::DispatchMotionEventWithSimultaneous(
    std::weak_ptr<GestureArenaMember> winner, float x, float y,
    std::shared_ptr<TouchEvent> lynx_touch_event,
    const ArkUI_UIInputEvent* event) {
  static std::vector<std::weak_ptr<GestureArenaMember>> empty_list;
  DispatchMotionEventWithSimultaneousAndReCompete(
      winner, x, y, lynx_touch_event, empty_list, event);
}

void GestureHandlerTrigger::DispatchMotionEventWithSimultaneousAndReCompete(
    std::weak_ptr<GestureArenaMember> winner, float x, float y,
    std::shared_ptr<TouchEvent> lynx_touch_event,
    std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
    const ArkUI_UIInputEvent* input_event) {
  if (!winner.lock()) {
    return;
  }

  DispatchMotionEventOnCurrentWinner(input_event, winner, lynx_touch_event, x,
                                     y);

  for (const auto& member : simultaneous_winners_) {
    DispatchMotionEventOnCurrentWinner(input_event, member, lynx_touch_event, x,
                                       y);
  }

  if (!compete_chain_candidates.empty()) {
    winner_ = ReCompeteByGestures(compete_chain_candidates, winner_);
  }
}

std::shared_ptr<BaseGestureHandler>
GestureHandlerTrigger::GetGestureHandlerById(
    std::weak_ptr<GestureArenaMember> member, int gesture_id) {
  auto handler_map = member.lock()->GetGestureHandlers();
  if (handler_map.empty()) {
    return nullptr;
  }

  for (auto& handler_pair : handler_map) {
    if (handler_pair.second->GetGestureDetector()->gesture_id() == gesture_id) {
      return handler_pair.second;
    }
  }

  return nullptr;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
