// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/detector/gesture_detector_manager.h"

#include <map>
#include <unordered_set>

#include "clay/ui/gesture_handler/arena/gesture_arena_manager.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"

namespace clay {

struct GestureArenaMemberCompare;

GestureDetectorManager::GestureDetectorManager(
    std::weak_ptr<GestureArenaManager> arena_manager)
    : arena_manager_(arena_manager) {}

void GestureDetectorManager::RegisterGestureIdWithMemberId(int gesture_id,
                                                           int member_id) {
  gesture_to_arena_members_[gesture_id].insert(member_id);
}

void GestureDetectorManager::UnregisterGestureIdWithMemberId(int gesture_id,
                                                             int member_id) {
  auto it = gesture_to_arena_members_.find(gesture_id);
  if (it != gesture_to_arena_members_.end()) {
    it->second.erase(member_id);
    if (it->second.empty()) {
      gesture_to_arena_members_.erase(it);
    }
  }
}

void GestureDetectorManager::RegisterGestureDetector(
    int member_id, std::shared_ptr<GestureDetector> gesture_detector) {
  if (!gesture_detector) {
    return;
  }
  RegisterGestureIdWithMemberId(gesture_detector->gesture_id(), member_id);
}

void GestureDetectorManager::UnregisterGestureDetector(
    int member_id, std::shared_ptr<GestureDetector> gesture_detector) {
  if (!gesture_detector) {
    return;
  }
  UnregisterGestureIdWithMemberId(gesture_detector->gesture_id(), member_id);
}

std::vector<fml::WeakPtr<GestureArenaMember>>
GestureDetectorManager::ConvertResponseChainToCompeteChain(
    const std::vector<fml::WeakPtr<GestureArenaMember>>& response_list) {
  std::vector<fml::WeakPtr<GestureArenaMember>> result;
  if (response_list.empty()) {
    return result;
  }

  for (const auto& node : response_list) {
    auto node_ptr = node.get();
    if (!node_ptr) {
      return result;
    }
    auto map = node_ptr->GetGestureDetectorMap();
    if (map.empty()) {
      result.push_back(node);
      continue;
    }

    std::vector<uint32_t> wait_for_list;
    std::vector<uint32_t> continue_with_list;

    std::map<int, std::shared_ptr<GestureDetector>> sorted_map(map.begin(),
                                                               map.end());

    for (const auto& entry : sorted_map) {
      const auto& relation_map = entry.second->relation_map();
      auto wait_for = relation_map.find("waitFor");
      auto continue_with = relation_map.find("continueWith");

      if (wait_for != relation_map.end()) {
        wait_for_list = wait_for->second;
      }
      if (continue_with != relation_map.end()) {
        continue_with_list = continue_with->second;
      }

      if (!wait_for_list.empty() || !continue_with_list.empty()) {
        break;
      }
    }

    if (wait_for_list.empty() && continue_with_list.empty()) {
      result.push_back(node);
      continue;
    }

    if (!wait_for_list.empty()) {
      std::vector<int> arena_members;
      for (int index : wait_for_list) {
        if (auto iter = gesture_to_arena_members_.find(index);
            iter != gesture_to_arena_members_.end()) {
          for (int member_id : iter->second) {
            if (std::find(arena_members.begin(), arena_members.end(),
                          member_id) == arena_members.end()) {
              // avoid duplicate add same member id
              arena_members.push_back(member_id);
            }
          }
        }
      }
      auto index_list =
          FindCandidatesAfterCurrentInChain(response_list, node, arena_members);
      if (index_list.empty()) {
        result.push_back(node);
        continue;
      }
      for (int index : index_list) {
        auto it = std::next(response_list.begin(), index);
        if (it != response_list.end()) {
          result.push_back(*it);
        }
      }
      result.push_back(node);
      break;
    } else {
      result.push_back(node);
    }

    if (!continue_with_list.empty()) {
      std::vector<int> arena_members;
      for (int index : continue_with_list) {
        if (auto iter = gesture_to_arena_members_.find(index);
            iter != gesture_to_arena_members_.end()) {
          for (int member_id : iter->second) {
            if (std::find(arena_members.begin(), arena_members.end(),
                          member_id) == arena_members.end()) {
              // avoid duplicate add same member id
              arena_members.push_back(member_id);
            }
          }
        }
      }
      auto continue_with_members = FindCandidatesFromArenaMember(
          gesture_to_arena_members_, node, arena_members);
      result.insert(result.end(), continue_with_members.begin(),
                    continue_with_members.end());
      break;
    }
  }

  return result;
}

std::pair<std::set<fml::WeakPtr<GestureArenaMember>, GestureArenaMemberCompare>,
          std::unordered_set<int>>
GestureDetectorManager::HandleSimultaneousWinner(
    fml::WeakPtr<GestureArenaMember> current) {
  if (!current || !arena_manager_.lock()) {
    return std::pair<
        std::set<fml::WeakPtr<GestureArenaMember>, GestureArenaMemberCompare>,
        std::unordered_set<int>>();
  }

  auto map = current->GetGestureDetectorMap();
  if (map.empty()) {
    return std::pair<
        std::set<fml::WeakPtr<GestureArenaMember>, GestureArenaMemberCompare>,
        std::unordered_set<int>>();
  }

  std::set<fml::WeakPtr<GestureArenaMember>, GestureArenaMemberCompare> results;
  std::unordered_set<int> current_gesture_ids;
  std::unordered_set<int> simultaneous_gesture_ids;

  for (const auto& entry : map) {
    current_gesture_ids.insert(entry.second->gesture_id());
  }

  for (const auto& entry : map) {
    if (!entry.second) {
      continue;
    }
    const auto& relation_map = entry.second->relation_map();
    auto simultaneous_list = relation_map.find("simultaneous");
    if (simultaneous_list == relation_map.end() ||
        simultaneous_list->second.empty()) {
      continue;
    }

    for (int gesture_id : simultaneous_list->second) {
      if (current_gesture_ids.find(gesture_id) != current_gesture_ids.end()) {
        simultaneous_gesture_ids.insert(gesture_id);
        continue;
      }

      auto member_set = gesture_to_arena_members_.find(gesture_id);
      if (member_set == gesture_to_arena_members_.end()) {
        continue;
      }

      for (int member_id : member_set->second) {
        auto member = arena_manager_.lock()->GetMemberById(member_id);
        if (!member ||
            member->GestureArenaMemberId() == current->GestureArenaMemberId()) {
          continue;
        }
        results.insert(member);
      }
    }
  }

  return {results, simultaneous_gesture_ids};
}

GestureDetectorManager::~GestureDetectorManager() { Destroy(); }

void GestureDetectorManager::Destroy() { gesture_to_arena_members_.clear(); }

std::vector<int> GestureDetectorManager::FindCandidatesAfterCurrentInChain(
    const std::vector<fml::WeakPtr<GestureArenaMember>>& response_list,
    fml::WeakPtr<GestureArenaMember> current,
    const std::vector<int>& arena_members) {
  std::vector<int> index_list;

  auto current_pos =
      std::find_if(response_list.begin(), response_list.end(),
                   [&current](const fml::WeakPtr<GestureArenaMember>& member) {
                     return member == current;
                   });

  for (int member_id : arena_members) {
    auto it = std::find_if(
        std::next(current_pos), response_list.end(),
        [member_id](const fml::WeakPtr<GestureArenaMember>& member) {
          return member && member->GestureArenaMemberId() == member_id;
        });
    if (it != response_list.end()) {
      index_list.push_back(std::distance(response_list.begin(), it));
    }
  }

  return index_list;
}

std::vector<fml::WeakPtr<GestureArenaMember>>
GestureDetectorManager::FindCandidatesFromArenaMember(
    const std::unordered_map<int, std::unordered_set<int>>&
        gesture_to_arena_members,
    fml::WeakPtr<GestureArenaMember> current,
    const std::vector<int>& arena_members) {
  std::vector<fml::WeakPtr<GestureArenaMember>> result_list;

  for (int index : arena_members) {
    auto member = arena_manager_.lock()->GetMemberById(index);
    if (member) {
      result_list.push_back(member);
    }
  }

  return result_list;
}

}  // namespace clay
