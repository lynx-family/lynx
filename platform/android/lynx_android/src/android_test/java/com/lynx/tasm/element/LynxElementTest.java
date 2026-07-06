// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.element;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxElementTest {
  @Test
  public void statusEnumContainsAliveAndDetached() {
    assertEquals("ALIVE", LynxElementStatus.ALIVE.name());
    assertEquals("DETACHED", LynxElementStatus.DETACHED.name());
  }

  @Test
  public void callbackReceivesValue() {
    AtomicReference<Integer> value = new AtomicReference<>();
    LynxElementCallback<Integer> callback = value::set;
    callback.onResult(7);
    assertEquals(Integer.valueOf(7), value.get());
  }

  @Test
  public void signIsReturnedFromHandle() {
    LynxElement element = new LynxElement(new LynxElementOwner(null, null), 7);
    AtomicReference<Integer> sign = new AtomicReference<>();

    element.getSign(sign::set);

    assertEquals(Integer.valueOf(7), sign.get());
  }

  @Test
  public void nullOwnerFallsBackToDetachedValues() {
    LynxElement element = new LynxElement(new LynxElementOwner(null, null), 7);
    AtomicReference<LynxElementStatus> status = new AtomicReference<>();
    AtomicReference<String> tagName = new AtomicReference<>("unset");
    AtomicReference<LynxElement> parent = new AtomicReference<>(element);
    AtomicReference<List<LynxElement>> children = new AtomicReference<>();
    AtomicReference<Map<String, Object>> positionInfo = new AtomicReference<>();
    AtomicReference<Map<String, Object>> dataset = new AtomicReference<>();
    AtomicReference<Map<String, Object>> attributes = new AtomicReference<>();
    AtomicReference<Object> attribute = new AtomicReference<>("unset");
    AtomicReference<String> json = new AtomicReference<>();

    element.getStatus(status::set);
    element.getTagName(tagName::set);
    element.getParent(parent::set);
    element.getChildren(children::set);
    element.getPositionInfo(positionInfo::set);
    element.getDataset(dataset::set);
    element.getAttributes(attributes::set);
    element.getAttribute("mode", attribute::set);
    element.toJSONString(json::set);

    assertEquals(LynxElementStatus.DETACHED, status.get());
    assertNull(tagName.get());
    assertNull(parent.get());
    assertNotNull(children.get());
    assertEquals(0, children.get().size());
    assertNotNull(positionInfo.get());
    assertTrue(positionInfo.get().isEmpty());
    assertNotNull(dataset.get());
    assertTrue(dataset.get().isEmpty());
    assertNotNull(attributes.get());
    assertTrue(attributes.get().isEmpty());
    assertNull(attribute.get());
    assertNull(json.get());
  }

  @Test
  public void filterTraversalHandlesDetachedChildren() {
    LynxElement element = new LynxElement(new LynxElementOwner(null, null), 7);
    AtomicReference<List<LynxElement>> matches = new AtomicReference<>();

    element.findNodesByFilter(candidate -> true, matches::set);

    assertNotNull(matches.get());
    assertEquals(1, matches.get().size());
    assertEquals(element, matches.get().get(0));
  }

  @Test
  public void platformViewCanReturnNull() {
    AtomicBoolean called = new AtomicBoolean(false);
    AtomicReference<View> view = new AtomicReference<>();
    LynxElement element = new LynxElement(new LynxElementOwner(null, null), 7);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> element.getPlatformView(result -> {
          called.set(true);
          view.set(result);
        }));

    assertTrue(called.get());
    assertNull(view.get());
  }
}
