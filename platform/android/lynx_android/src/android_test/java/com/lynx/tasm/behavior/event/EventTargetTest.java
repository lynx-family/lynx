// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.event;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.event.EventsListener;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;

public class EventTargetTest {
  // A compiled implementor preserves default-method forwarding after Android desugaring.
  public abstract static class TestEventTarget implements EventTarget {}

  @Test
  public void baseTargetHasNoEventListenersByDefault() {
    EventTargetBase target = new EventTargetBase() {
      @Override
      public EventTargetBase parentResponder() {
        return null;
      }

      @Override
      public ReadableMap getDataset() {
        return null;
      }
    };

    assertFalse(target.hasEventListener("tap"));
  }

  @Test
  public void targetWithoutEventsHasNoListener() {
    EventTargetBase target = mock(TestEventTarget.class, CALLS_REAL_METHODS);

    assertFalse(target.hasEventListener("tap"));
  }

  @Test
  public void hasEventListenerMatchesEventName() {
    EventTarget target = mock(TestEventTarget.class, CALLS_REAL_METHODS);
    Map<String, EventsListener> events = new HashMap<>();
    events.put("tap", new EventsListener("tap", "bindEvent", "onTap", null, null));
    when(target.getEvents()).thenReturn(events);
    EventTargetBase base = target;

    assertTrue(base.hasEventListener("tap"));
    assertFalse(base.hasEventListener("click"));
  }

  @Test
  public void hasEventListenerRecognizesTextServiceNullPlaceholder() {
    EventTarget target = mock(TestEventTarget.class, CALLS_REAL_METHODS);
    Map<String, EventsListener> events = new HashMap<>();
    events.put("tap", null);
    when(target.getEvents()).thenReturn(events);

    assertTrue(target.hasEventListener("tap"));
    assertFalse(target.hasEventListener("click"));
  }

  @Test
  public void hasEventListenerIncludesLepusAndGlobalBindings() {
    EventTarget target = mock(TestEventTarget.class, CALLS_REAL_METHODS);
    Map<String, EventsListener> events = new HashMap<>();
    events.put("tap", new EventsListener("tap", null, null, "capture-bind", "onTap"));
    events.put("custom", new EventsListener("custom", "global-bindEvent", "onCustom", null, null));
    when(target.getEvents()).thenReturn(events);

    assertTrue(target.hasEventListener("tap"));
    assertTrue(target.hasEventListener("custom"));
  }

  @Test
  public void hasEventListenerReflectsBindingUpdatesAndRemoval() {
    EventTarget target = mock(TestEventTarget.class, CALLS_REAL_METHODS);
    Map<String, EventsListener> events = new HashMap<>();
    when(target.getEvents()).thenReturn(events);

    assertFalse(target.hasEventListener("tap"));
    events.put("tap", null);
    assertTrue(target.hasEventListener("tap"));
    events.clear();
    assertFalse(target.hasEventListener("tap"));
    when(target.getEvents()).thenReturn(null);
    assertFalse(target.hasEventListener("tap"));
  }
}
