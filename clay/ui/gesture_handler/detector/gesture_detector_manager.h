// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_DETECTOR_GESTURE_DETECTOR_MANAGER_H_
#define CLAY_UI_GESTURE_HANDLER_DETECTOR_GESTURE_DETECTOR_MANAGER_H_

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"

namespace clay {

class GestureDetector;

class GestureArenaMember;
class GestureArenaManager;
struct GestureArenaMemberCompare;

/**
 * The GestureDetectorManager class manages the association between gesture
 * detectors and arena members. It provides methods to register and unregister
 * gesture detectors for specific members, convert response chains to competitor
 * chains based on gesture relationships, handle simultaneous winners, and
 * perform other related operations.
 *
 * The class utilizes a map to store the mapping between gesture IDs and sets of
 * associated member IDs. It supports the registration and un-registration of
 * gesture detectors, allowing multiple gesture detectors to be associated with
 * the same member. It also provides functionality to convert a response chain
 * to a competitor chain, considering gesture relationships such as waiting for
 * other gestures before execution.
 *
 * The class assumes that the associated GestureArenaMember objects and
 * GestureDetector objects are properly managed and provided externally. It does
 * not handle the actual logic of gesture detection or arena management.
 *
 */
class GestureDetectorManager {
 public:
  explicit GestureDetectorManager(
      std::weak_ptr<GestureArenaManager> arena_manager);

  ~GestureDetectorManager();

  void Destroy();

  void RegisterGestureIdWithMemberId(int gesture_id, int member_id);
  void UnregisterGestureIdWithMemberId(int gesture_id, int member_id);
  void RegisterGestureDetector(
      int member_id, std::shared_ptr<GestureDetector> gesture_detector);
  void UnregisterGestureDetector(
      int member_id, std::shared_ptr<GestureDetector> gesture_detector);

  std::vector<fml::WeakPtr<GestureArenaMember>>
  ConvertResponseChainToCompeteChain(
      const std::vector<fml::WeakPtr<GestureArenaMember>>& response_list);

  std::pair<
      std::set<fml::WeakPtr<GestureArenaMember>, GestureArenaMemberCompare>,
      std::unordered_set<int>>
  HandleSimultaneousWinner(fml::WeakPtr<GestureArenaMember> current);

 private:
  std::vector<int> FindCandidatesAfterCurrentInChain(
      const std::vector<fml::WeakPtr<GestureArenaMember>>& response_list,
      fml::WeakPtr<GestureArenaMember> current,
      const std::vector<int>& arena_members);
  std::vector<fml::WeakPtr<GestureArenaMember>> FindCandidatesFromArenaMember(
      const std::unordered_map<int, std::unordered_set<int>>&
          gesture_to_arena_members,
      fml::WeakPtr<GestureArenaMember> current,
      const std::vector<int>& arena_members);
  // key —— gesture id, value —— the set of member id
  std::unordered_map<int, std::unordered_set<int>> gesture_to_arena_members_;
  std::weak_ptr<GestureArenaManager> arena_manager_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_DETECTOR_GESTURE_DETECTOR_MANAGER_H_
