// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.gesture.detector.GestureDetector;
import java.util.Map;

public class UIParams {
  public UIParams(int sign, int nodeIndex, boolean flatten, String tagName,
      StylesDiffMap initialProps, Map<String, EventsListener> eventsListenerMap,
      Map<Integer, GestureDetector> gestureDetectors) {
    mSign = sign;
    mNodeIndex = nodeIndex;
    mIsFlatten = flatten;

    mTagName = tagName;

    mInitialProps = initialProps;
    mEventsListenerMap = eventsListenerMap;
    mGestureDetectors = gestureDetectors;
  }

  public int mSign;
  public int mNodeIndex;

  public boolean mIsFlatten;

  public String mTagName;

  public @Nullable StylesDiffMap mInitialProps;
  public @Nullable Map<String, EventsListener> mEventsListenerMap;
  public @Nullable Map<Integer, GestureDetector> mGestureDetectors;
}
