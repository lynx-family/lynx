// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

public class LynxAccessibilityMutationHelper {
  private static final String TAG = "LynxAccessibilityMutationHelper";

  /** Mutation action insert */
  public static final int MUTATION_ACTION_INSERT = 0;

  /** Mutation action remove */
  public static final int MUTATION_ACTION_REMOVE = 1;

  /** Mutation action detach */
  public static final int MUTATION_ACTION_DETACH = 2;

  /** Mutation action update */
  public static final int MUTATION_ACTION_UPDATE = 3;

  /** Mutation action style update */
  public static final int MUTATION_ACTION_STYLE_UPDATE = 4;

  /** All mutation events */
  final protected ArrayList<JavaOnlyMap> mMutationEventList = new ArrayList<>();

  /** All mutation styles registered by LynxAccessibilityModule. */
  final protected Set<String> mMutationStyles = new HashSet<>();

  public void registerMutationStyle(final ReadableArray array) {
    if (array != null && mMutationStyles != null) {
      mMutationStyles.clear();
      for (int i = 0; i < array.size(); ++i) {
        if (array.getType(i) == ReadableType.String) {
          mMutationStyles.add(array.getString(i));
        }
      }
    }
  }

  public void insertA11yMutationEvent(final int action, final LynxBaseUI ui) {
    insertA11yMutationEvent(action, ui, "");
  }

  public void insertA11yMutationEvent(final int action, final LynxBaseUI ui, final String key) {
    if (ui != null && !mutationEventTypeToString(action).isEmpty()) {
      JavaOnlyMap event = new JavaOnlyMap();
      event.putInt("target", ui.getSign());
      event.putString("action", mutationEventTypeToString(action));
      event.putString("a11y-id", ui.getAccessibilityId());
      if (action == MUTATION_ACTION_STYLE_UPDATE) {
        if (!mMutationStyles.contains(key)) {
          return;
        }
        event.putString("style", key);
      }
      mMutationEventList.add(event);
    }
  }

  public void flushA11yMutationEvents(LynxContext context) {
    if (context != null && !mMutationEventList.isEmpty()) {
      JavaOnlyArray events = new JavaOnlyArray();
      for (JavaOnlyMap item : mMutationEventList) {
        events.add(item);
      }
      JavaOnlyArray params = new JavaOnlyArray();
      params.add(events);
      context.sendGlobalEvent("a11y-mutations", params);
      mMutationEventList.clear();
    }
  }

  private String mutationEventTypeToString(final int action) {
    switch (action) {
      case MUTATION_ACTION_INSERT:
        return "insert";
      case MUTATION_ACTION_REMOVE:
        return "remove";
      case MUTATION_ACTION_DETACH:
        return "detach";
      case MUTATION_ACTION_UPDATE:
        return "update";
      case MUTATION_ACTION_STYLE_UPDATE:
        return "style_update";
      default:
        return "";
    }
  }
}
