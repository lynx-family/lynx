// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.event.EventsListener;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;

public class EventTargetSpanTest {
  private EventTargetSpan createSpan(int sign) {
    return createSpan(sign, null);
  }

  private EventTargetSpan createSpan(int sign, Map<String, EventsListener> events) {
    return new EventTargetSpan(sign, events, EventTarget.EnableStatus.Undefined, true,
        EventTarget.EnableStatus.Undefined, EventTarget.PointerEventsValue.Unset, null);
  }

  @Test
  public void parentResponderTraversesSpanAndNonSpanAncestors() {
    EventTarget root = mock(EventTarget.class);
    EventTargetSpan parent = createSpan(2);
    EventTargetSpan child = createSpan(3);
    parent.setParent(root);
    child.setParent(parent);

    EventTargetBase current = child.parentResponder();
    assertSame(parent, current);
    current = current.parentResponder();
    assertSame(root, current);
    assertNull(current.parentResponder());
  }

  @Test
  public void parentResponderWithoutParentReturnsNull() {
    assertNull(createSpan(1).parentResponder());
  }

  @Test
  public void parentResponderAfterParentRemovalReturnsNull() {
    EventTargetSpan parent = createSpan(1);
    EventTargetSpan child = createSpan(2);
    child.setParent(parent);
    child.setParent(null);

    assertNull(child.parentResponder());
  }

  @Test
  public void hasEventListenerReadsSpanSnapshot() {
    Map<String, EventsListener> events = new HashMap<>();
    events.put("tap", null);
    EventTargetBase target = createSpan(1, events);
    events.clear();
    events.put("click", null);

    assertTrue(target.hasEventListener("tap"));
    assertFalse(target.hasEventListener("click"));
  }

  @Test
  public void hasEventListenerDoesNotIncludeAncestorBindings() {
    Map<String, EventsListener> events = new HashMap<>();
    events.put("tap", null);
    EventTargetSpan parent = createSpan(1, events);
    EventTargetSpan child = createSpan(2);
    child.setParent(parent);
    EventTargetBase target = child;

    assertFalse(target.hasEventListener("tap"));
    assertTrue(target.parentResponder().hasEventListener("tap"));
  }
}
