// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.detector;

import android.util.Pair;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import java.util.*;

/**
 * The GestureDetectorManager class manages the association between gesture detectors and arena
 * members. It provides methods to register and unregister gesture detectors for specific members,
 * convert response chains to competitor chains based on gesture relationships, handle simultaneous
 * winners, and perform other related operations.
 *
 * The class utilizes a map to store the mapping between gesture IDs and sets of associated member
 * IDs. It supports the registration and un-registration of gesture detectors, allowing multiple
 * gesture detectors to be associated with the same member. It also provides functionality to
 * convert a response chain to a competitor chain, considering gesture relationships such as waiting
 * for other gestures before execution.
 *
 * The class assumes that the associated GestureArenaMember objects and GestureDetector objects are
 * properly managed and provided externally. It does not handle the actual logic of gesture
 * detection or arena management.
 *
 */
public class GestureDetectorManager {
  // key —— gesture id, value —— the set of member id
  private final Map<Integer, Set<Integer>> mGestureToArenaMembers;
  private final GestureArenaManager mArenaManager;

  /**
   * Constructs a GestureDetectorManager.
   */
  public GestureDetectorManager(GestureArenaManager arenaManager) {
    mGestureToArenaMembers = new HashMap<>();
    mArenaManager = arenaManager;
  }

  /**
   * Registers a gesture ID with a member ID.
   *
   * @param gestureId The ID of the gesture.
   * @param memberId The ID of the member.
   */
  @MainThread
  private void registerGestureIdWithMemberId(int gestureId, int memberId) {
    if (mGestureToArenaMembers.get(gestureId) == null) {
      mGestureToArenaMembers.put(gestureId, new HashSet<>());
    }
    Set set = mGestureToArenaMembers.get(gestureId);
    set.add(memberId);
  }

  /**
   * Unregisters a gesture ID with a member ID.
   *
   * @param gestureId The ID of the gesture.
   * @param memberId The ID of the member.
   */
  @MainThread
  private void unregisterGestureIdWithMemberId(int gestureId, int memberId) {
    Set<Integer> members = mGestureToArenaMembers.get(gestureId);
    if (members == null || members.isEmpty() || !members.contains(memberId)) {
      return;
    }
    members.remove(memberId);
    if (members.isEmpty()) {
      mGestureToArenaMembers.remove(gestureId);
    }
  }

  /**
   * Registers a gesture detector for a member.
   *
   * @param memberId The ID of the member.
   * @param gestureDetector The gesture detector to register.
   */
  @MainThread
  public void registerGestureDetector(int memberId, GestureDetector gestureDetector) {
    if (gestureDetector == null) {
      return;
    }
    registerGestureIdWithMemberId(gestureDetector.getGestureID(), memberId);
  }

  /**
   * Unregisters a gesture detector for a member.
   *
   * @param memberId The ID of the member.
   * @param gestureDetector The gesture detector to unregister.
   */
  @MainThread
  public void unregisterGestureDetector(int memberId, GestureDetector gestureDetector) {
    if (gestureDetector == null) {
      return;
    }
    unregisterGestureIdWithMemberId(gestureDetector.getGestureID(), memberId);
  }

  /**
   * Converts the response chain to a competitor chain based on gesture relationships.
   *
   * @param responseList The response chain to convert.
   * @return The converted compete chain.
   */
  public LinkedList<GestureArenaMember> convertResponseChainToCompeteChain(
      LinkedList<GestureArenaMember> responseList) {
    LinkedList<GestureArenaMember> result = new LinkedList<>();
    if (responseList == null || responseList.isEmpty()) {
      return result;
    }
    for (int i = 0; i < responseList.size(); i++) {
      GestureArenaMember node = responseList.get(i);
      Map<Integer, GestureDetector> map = node.getGestureDetectorMap();
      if (map == null) {
        // keep the original response chain relationship
        result.add(node);
        continue;
      }

      List<Integer> waitForList = null;
      List<Integer> continueWithList = null;

      Map<Integer, GestureDetector> sortedMap = new TreeMap<>(map);

      for (Map.Entry<Integer, GestureDetector> entry : sortedMap.entrySet()) {
        // TODO(luochangan.adrian): Gesture relations need distinguish between types
        waitForList = entry.getValue().getRelationMap().get(GestureDetector.WAIT_FOR);
        continueWithList = entry.getValue().getRelationMap().get(GestureDetector.CONTINUE_WITH);
        if ((waitForList != null && !waitForList.isEmpty())
            || (continueWithList != null && !continueWithList.isEmpty())) {
          // when waitForList or continueWithList not empty, exit loop
          break;
        }
      }

      if ((continueWithList == null || continueWithList.isEmpty())
          && (waitForList == null || waitForList.isEmpty())) {
        // keep the original response chain relationship
        result.add(node);
        continue;
      }
      if (waitForList != null && !waitForList.isEmpty()) {
        // handle waitFor relation
        LinkedHashSet<Integer> arenaMembers = new LinkedHashSet<>();
        for (Integer index : waitForList) {
          if (mGestureToArenaMembers.get(index) != null) {
            arenaMembers.addAll(mGestureToArenaMembers.get(index));
          }
        }
        LinkedList<Integer> indexList =
            findCandidatesAfterCurrentInChain(responseList, node, arenaMembers);
        if (indexList == null || indexList.isEmpty()) {
          result.add(node);
          continue;
        }
        for (Integer index : indexList) {
          if (responseList.get(index) != null) {
            result.add(responseList.get(index));
          }
        }
        result.add(node);
        // skip to the last GestureArenaMember
        i = responseList.size();
      } else {
        result.add(node);
      }

      if (continueWithList != null && !continueWithList.isEmpty()) {
        // handle continueWith relation
        LinkedHashSet<Integer> arenaMembers = new LinkedHashSet<>();
        for (Integer index : continueWithList) {
          if (mGestureToArenaMembers.get(index) != null) {
            arenaMembers.addAll(mGestureToArenaMembers.get(index));
          }
        }
        LinkedList<GestureArenaMember> continueWithMembers =
            findCandidatesFromArenaMember(mGestureToArenaMembers, node, arenaMembers);
        if (continueWithMembers != null && !continueWithMembers.isEmpty()) {
          result.addAll(continueWithMembers);
        }
        break;
      }
    }
    return result;
  }

  /**
   * Handles simultaneous winners and returns a list of affected members.
   *
   * @param current The current GestureArenaMember.
   * @return The list of simultaneous winners.
   */
  @Nullable
  public Pair<HashSet<GestureArenaMember>, HashSet<Integer>> handleSimultaneousWinner(
      GestureArenaMember current) {
    if (current == null || mArenaManager == null) {
      return null;
    }
    Map<Integer, GestureDetector> map = current.getGestureDetectorMap();
    if (map == null) {
      return null;
    }
    HashSet<GestureArenaMember> results = new HashSet<>();
    HashSet<Integer> currentGestureIds = new HashSet<>();
    HashSet<Integer> simultaneousGestureIds = new HashSet<>();

    for (Map.Entry<Integer, GestureDetector> entry : map.entrySet()) {
      currentGestureIds.add(entry.getValue().getGestureID());
    }

    for (Map.Entry<Integer, GestureDetector> entry : map.entrySet()) {
      if (entry.getValue() == null) {
        continue;
      }
      List<Integer> simultaneousList =
          entry.getValue().getRelationMap().get(GestureDetector.SIMULTANEOUS);
      if (simultaneousList == null || simultaneousList.isEmpty()) {
        continue;
      }
      for (int i : simultaneousList) {
        if (currentGestureIds.contains(i)) {
          simultaneousGestureIds.add(i);
          continue;
        }

        Set<Integer> memberSets = mGestureToArenaMembers.get(i);
        if (memberSets == null) {
          continue;
        }
        for (int memberId : memberSets) {
          GestureArenaMember member = mArenaManager.getMemberById(memberId);
          if (member == null
              || member.getGestureArenaMemberId() == current.getGestureArenaMemberId()) {
            continue;
          }
          results.add(member);
        }
      }
    }
    return new Pair<>(results, simultaneousGestureIds);
  }

  /**
   * Finds the candidates in the response chain after the current node.
   *
   * @param responseList The response chain.
   * @param current The current GestureArenaMember.
   * @param arenaMembers The set of arena members to check for candidates.
   * @return The list of candidate indices.
   */
  @Nullable
  private LinkedList<Integer> findCandidatesAfterCurrentInChain(
      LinkedList<GestureArenaMember> responseList, GestureArenaMember current,
      LinkedHashSet<Integer> arenaMembers) {
    if (responseList == null || arenaMembers == null || current == null) {
      return null;
    }

    int index = responseList.lastIndexOf(current);
    if (index < 0 || index >= responseList.size()) {
      return null;
    }
    LinkedList<Integer> indexList = new LinkedList<>();
    // Given the response chain: A -> B -> C, if the following rules exist:
    // A waitFor C, A waitFor B, the expected final wait chain is C -> B -> A
    // A waitFor B, A waitFor C, the expected final wait chain is B -> C -> A
    for (int id : arenaMembers) {
      // add to index list in order
      for (int i = index + 1; i < responseList.size(); i++) {
        if (id == responseList.get(i).getGestureArenaMemberId()) {
          indexList.add(i);
        }
      }
    }
    return indexList;
  }

  private LinkedList<GestureArenaMember> findCandidatesFromArenaMember(
      Map<Integer, Set<Integer>> gestureToArenaMembers, GestureArenaMember current,
      Set<Integer> arenaMembers) {
    if (gestureToArenaMembers == null || arenaMembers == null || current == null
        || mArenaManager == null) {
      return null;
    }
    LinkedList<GestureArenaMember> resultList = new LinkedList<>();

    for (Integer index : arenaMembers) {
      resultList.add(mArenaManager.getMemberById(index));
    }
    return resultList;
  }

  /**
   * Clears the gestureToArenaMembers map.
   */
  public void onDestroy() {
    mGestureToArenaMembers.clear();
  }
}
