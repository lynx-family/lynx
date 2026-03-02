// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/gesture_handler_trigger.h"

#include <stdint.h>

#include <cfloat>
#include <cstddef>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/detector/gesture_detector_manager.h"
#include "clay/ui/gesture_handler/gesture_handler_delegate.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

GestureHandlerTrigger::GestureHandlerTrigger(
    PageView* page_view, std::shared_ptr<GestureDetectorManager> manager)
    : page_view_(page_view),
      last_fling_target_id_(0),
      velocity_x_(0),
      velocity_y_(0),
      gesture_detector_manager_(manager),
      fling_scroller_(
          std::make_unique<FlingScroller>(page_view->GetAnimationHandler())) {}

void GestureHandlerTrigger::InitCurrentWinnerWhenDown(
    fml::WeakPtr<GestureArenaMember> member) {
  winner_ = member;
  UpdateLastWinner(winner_);
  UpdateSimultaneousWinner(winner_);
  ResetGestureHandlerAndSimultaneous(winner_);
  current_extra_bundle_ = std::make_shared<GestureExtraBundle>();
}

void GestureHandlerTrigger::ResolveTouchEvent(
    const PointerEvent* pointer_event,
    std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
    std::vector<fml::WeakPtr<GestureArenaMember>>& bubble_chain_candidates) {
  PointerEvent::EventType touch_action = pointer_event->type;
  FloatPoint page_point = pointer_event->position;

  if (touch_action == PointerEvent::EventType::kDownEvent) {
    ResetCandidatesGestures(compete_chain_candidates);
    StopFlingByLastFlingMember(pointer_event, compete_chain_candidates,
                               bubble_chain_candidates);
    DispatchMotionEventWithSimultaneousAndReCompete(
        winner_, page_point.x(), page_point.y(), compete_chain_candidates,
        pointer_event);
    FindNextWinnerInBegin(compete_chain_candidates, page_point.x(),
                          page_point.y(), pointer_event);

  } else if (touch_action == PointerEvent::EventType::kMoveEvent) {
    winner_ = ReCompeteByGestures(compete_chain_candidates, winner_);
    if (winner_ == last_winner_) {
      DispatchMotionEventWithSimultaneous(winner_, page_point.x(),
                                          page_point.y(), pointer_event);
    }
    FindNextWinnerInBegin(compete_chain_candidates, page_point.x(),
                          page_point.y(), pointer_event);
  } else if (touch_action == PointerEvent::EventType::kUpEvent ||
             touch_action == PointerEvent::EventType::kCancel) {
    DispatchMotionEventWithSimultaneousAndReCompete(
        winner_, page_point.x(), page_point.y(), compete_chain_candidates,
        pointer_event);
    if (winner_ &&
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
          winner_, FLT_MIN, FLT_MIN, compete_chain_candidates, nullptr);
    }
  }
}

void GestureHandlerTrigger::SetVelocity(float velocity_x, float velocity_y) {
  velocity_x_ = velocity_x;
  velocity_y_ = velocity_y;
}

void GestureHandlerTrigger::FlingCallback(int status, float delta_x,
                                          float delta_y) {
  delta_x = delta_x / page_view_->DevicePixelRatio();
  delta_y = delta_y / page_view_->DevicePixelRatio();

  winner_ = ReCompeteByGestures(compete_chain_candidates_, winner_);
  FindNextWinnerInBegin(compete_chain_candidates_, 0, 0, nullptr);
  if (winner_) {
    last_fling_target_id_ = winner_->GestureArenaMemberId();
    DispatchMotionEventWithSimultaneous(winner_, delta_x, delta_y, nullptr);
    if (fling_scroller_->IsIdle()) {
      DispatchMotionEventWithSimultaneousAndReCompete(
          winner_, FLT_MIN, FLT_MIN, compete_chain_candidates_, nullptr);
    }
  } else {
    last_fling_target_id_ = 0;
    if (!fling_scroller_->IsIdle()) {
      fling_scroller_->Stop();
    }
  }
}

void GestureHandlerTrigger::HandleGestureDetectorState(
    fml::WeakPtr<GestureArenaMember> member, uint32_t gesture_id, int state) {
  if (!member) {
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
    const PointerEvent* pointer_event,
    std::vector<fml::WeakPtr<GestureArenaMember>>& bubble_candidate,
    fml::WeakPtr<GestureArenaMember> winner) {
  PointerEvent::EventType type = pointer_event->type;
  if (!winner) {
    return;
  }

  if (type != PointerEvent::EventType::kDownEvent &&
      type != PointerEvent::EventType::kMoveEvent &&
      type != PointerEvent::EventType::kCancel &&
      type != PointerEvent::EventType::kUpEvent) {
    return;
  }

  for (const auto& member : bubble_candidate) {
    auto gesture_handler_map = member->GetGestureHandlers();
    if (gesture_handler_map.empty()) {
      continue;
    }
    for (const auto& handler_entry : gesture_handler_map) {
      auto& handler = handler_entry.second;
      if (type == PointerEvent::EventType::kDownEvent) {
        handler->OnTouchesDown(pointer_event);
      } else if (type == PointerEvent::EventType::kMoveEvent) {
        handler->OnTouchesMove(pointer_event);
      } else if (type == PointerEvent::EventType::kUpEvent) {
        handler->OnTouchesUp(pointer_event);
      } else if (type == PointerEvent::EventType::kCancel) {
        handler->OnTouchesCancel(pointer_event);
      }
    }
  }
}

void GestureHandlerTrigger::Destroy() {
  page_view_ = nullptr;
  if (!simultaneous_winners_.empty()) {
    simultaneous_winners_.clear();
  }
}

void GestureHandlerTrigger::ResetCandidatesGestures(
    std::vector<fml::WeakPtr<GestureArenaMember>>& members) {
  for (const auto& member : members) {
    ResetGestureHandlerAndSimultaneous(member);
  }
  duplicated_member_ = fml::WeakPtr<GestureArenaMember>();
}

void GestureHandlerTrigger::FailOthersMembersInRaceRelation(
    fml::WeakPtr<GestureArenaMember> member, uint32_t current_gesture_id,
    std::unordered_set<int>& simultaneous_gesture_ids) {
  if (!member) {
    return;
  }
  auto gesture_handler_map = member->GetGestureHandlers();
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
    const PointerEvent* pointer_event,
    std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
    std::vector<fml::WeakPtr<GestureArenaMember>>& bubble_candidates) {
  if (bubble_candidates.empty()) {
    return;
  }
  for (const auto& member : bubble_candidates) {
    if ((winner_ && last_fling_target_id_ == member->GestureArenaMemberId()) ||
        last_fling_target_id_ == 0) {
      last_fling_target_id_ = 0;
      if (!fling_scroller_->IsIdle()) {
        DispatchMotionEventWithSimultaneousAndReCompete(
            winner_, 0, 0, compete_chain_candidates, pointer_event);
        fling_scroller_->Stop();
        if (page_view_) {
          page_view_->OnGestureRecognizedWithSign(member->Sign());
        }
      }
      break;
    }
  }
}

void GestureHandlerTrigger::FindNextWinnerInBegin(
    std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
    float x, float y, const PointerEvent* event) {
  for (size_t i = 0; i < compete_chain_candidates.size(); ++i) {
    if (winner_ == last_winner_ || !winner_) {
      return;
    }
    UpdateLastWinner(winner_);
    UpdateSimultaneousWinner(winner_);
    DispatchMotionEventWithSimultaneousAndReCompete(
        winner_, x, y, compete_chain_candidates, event);
  }
  DispatchMotionEventWithSimultaneousAndReCompete(
      last_winner_, x, y, compete_chain_candidates, event);
}

void GestureHandlerTrigger::UpdateSimultaneousWinner(
    fml::WeakPtr<GestureArenaMember> winner) {
  auto result = gesture_detector_manager_->HandleSimultaneousWinner(winner);
  simultaneous_winners_ = result.first;
  simultaneous_gesture_ids_ = result.second;
}

void GestureHandlerTrigger::UpdateLastWinner(
    fml::WeakPtr<GestureArenaMember> winner) {
  if (last_winner_ != winner) {
    last_winner_ = winner;
  }
}

fml::WeakPtr<GestureArenaMember> GestureHandlerTrigger::ReCompeteByGestures(
    std::vector<fml::WeakPtr<GestureArenaMember>>& competitor_chain_candidates,
    fml::WeakPtr<GestureArenaMember> current) {
  if ((!current && !last_winner_) || !competitor_chain_candidates.size()) {
    return fml::WeakPtr<GestureArenaMember>();
  }

  bool need_reCompete_last_winner = false;
  if (!current && last_winner_) {
    need_reCompete_last_winner = true;
    current = last_winner_;
    ResetGestureHandlerAndSimultaneous(last_winner_);
  }

  int state_current = GetCurrentMemberState(current);
  if (state_current <= GestureConstants::LYNX_STATE_ACTIVE) {
    return current;
  } else if (state_current == GestureConstants::LYNX_STATE_END) {
    return fml::WeakPtr<GestureArenaMember>();
  }

  if (need_reCompete_last_winner) {
    return fml::WeakPtr<GestureArenaMember>();
  }

  auto it = std::find_if(
      competitor_chain_candidates.begin(), competitor_chain_candidates.end(),
      [&current](const fml::WeakPtr<GestureArenaMember>& elem) {
        auto current_lock = current;
        auto elem_lock = elem;
        if (!current_lock || !elem_lock) {
          return false;
        }
        return current_lock->GestureArenaMemberId() ==
               elem_lock->GestureArenaMemberId();
      });

  if (it == competitor_chain_candidates.end()) {
    return fml::WeakPtr<GestureArenaMember>();
  }

  auto last_it =
      std::find_if(competitor_chain_candidates.rbegin(),
                   competitor_chain_candidates.rend(),
                   [&current](const fml::WeakPtr<GestureArenaMember>& elem) {
                     auto current_lock = current;
                     auto elem_lock = elem;
                     return current_lock && elem_lock &&
                            current_lock->GestureArenaMemberId() ==
                                elem_lock->GestureArenaMemberId();
                   })
          .base() -
      1;

  if (it != last_it) {
    if (duplicated_member_) {
      it = last_it - 1;
      ResetGestureHandlerAndSimultaneous(duplicated_member_);
    } else {
      duplicated_member_ = *it;
    }
  }
  if (it == competitor_chain_candidates.end() ||
      it < competitor_chain_candidates.begin() ||
      it >= competitor_chain_candidates.end()) {
    return fml::WeakPtr<GestureArenaMember>();
  }

  int current_member_id = (*it)->GestureArenaMemberId();

  for (auto next_it = it + 1; next_it != competitor_chain_candidates.end();
       ++next_it) {
    auto node = *next_it;
    if (node->GestureArenaMemberId() == current_member_id) {
      continue;
    }
    if (duplicated_member_ == node) {
      duplicated_member_.reset();
    } else {
      if (GetCurrentMemberState(node) != GestureConstants::LYNX_STATE_END) {
        ResetGestureHandlerAndSimultaneous(node);
      }
    }

    int state = GetCurrentMemberState(node);
    if (state <= GestureConstants::LYNX_STATE_ACTIVE) {
      return node;
    } else if (state == GestureConstants::LYNX_STATE_END) {
      return fml::WeakPtr<GestureArenaMember>();
    }
  }

  for (auto pre_it = competitor_chain_candidates.begin(); pre_it != it;
       ++pre_it) {
    auto node = *pre_it;
    if (node->GestureArenaMemberId() == current_member_id) {
      continue;
    }
    if (duplicated_member_ == node) {
      duplicated_member_.reset();
    } else {
      if (GetCurrentMemberState(node) != GestureConstants::LYNX_STATE_END) {
        ResetGestureHandlerAndSimultaneous(node);
      }
    }

    int state = GetCurrentMemberState(node);
    if (state <= GestureConstants::LYNX_STATE_ACTIVE) {
      return node;
    } else if (state == GestureConstants::LYNX_STATE_END) {
      return fml::WeakPtr<GestureArenaMember>();
    }
  }

  return fml::WeakPtr<GestureArenaMember>();
}

void GestureHandlerTrigger::DispatchMotionEventOnCurrentWinner(
    const PointerEvent* event, fml::WeakPtr<GestureArenaMember> member,
    float delta_x, float delta_y, bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& current_extra_bundle) {
  if (!member) {
    return;
  }
  auto gesture_handler_map = member->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return;
  }

  for (auto& handler_pair : gesture_handler_map) {
    handler_pair.second->HandleMotionEvent(
        event, delta_x, delta_y, handle_by_simultaneous, current_extra_bundle);
  }
}

int GestureHandlerTrigger::GetCurrentMemberState(
    fml::WeakPtr<GestureArenaMember> node) {
  if (!node) {
    return GestureConstants::LYNX_STATE_FAIL;
  }

  auto gesture_handler_map = node->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return GestureConstants::LYNX_STATE_FAIL;
  }

  int min_status = -1;
  for (auto& handler_pair : gesture_handler_map) {
    auto& handler = handler_pair.second;
    if (handler->IsEnd()) {
      ResetGestureHandlerAndSimultaneous(node);
      last_winner_ = {};
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
    fml::WeakPtr<GestureArenaMember> member) {
  ResetGestureHandler(member);

  for (const auto& arena_member : simultaneous_winners_) {
    ResetGestureHandler(arena_member);
  }
}

void GestureHandlerTrigger::ResetGestureHandler(
    fml::WeakPtr<GestureArenaMember> member) {
  if (!member) {
    return;
  }

  auto gesture_handler_map = member->GetGestureHandlers();
  if (gesture_handler_map.empty()) {
    return;
  }

  for (auto& handler_pair : gesture_handler_map) {
    handler_pair.second->Reset();
  }
}

void GestureHandlerTrigger::DispatchMotionEventWithSimultaneous(
    fml::WeakPtr<GestureArenaMember> winner, float x, float y,
    const PointerEvent* event) {
  static std::vector<fml::WeakPtr<GestureArenaMember>> empty_list;
  DispatchMotionEventWithSimultaneousAndReCompete(winner, x, y, empty_list,
                                                  event);
}

void GestureHandlerTrigger::DispatchMotionEventWithSimultaneousAndReCompete(
    fml::WeakPtr<GestureArenaMember> winner, float x, float y,
    std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
    const PointerEvent* pointer_event) {
  if (!winner) {
    return;
  }

  DispatchMotionEventOnCurrentWinner(pointer_event, winner, x, y, false,
                                     current_extra_bundle_);

  for (const auto& member : simultaneous_winners_) {
    DispatchMotionEventOnCurrentWinner(pointer_event, member, x, y, true,
                                       current_extra_bundle_);
  }

  current_extra_bundle_->ResetSimultaneousDelta();

  if (!compete_chain_candidates.empty()) {
    winner_ = ReCompeteByGestures(compete_chain_candidates, winner_);
  }
}

std::shared_ptr<BaseGestureHandler>
GestureHandlerTrigger::GetGestureHandlerById(
    fml::WeakPtr<GestureArenaMember> member, uint32_t gesture_id) {
  auto handler_map = member->GetGestureHandlers();
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

}  // namespace clay
