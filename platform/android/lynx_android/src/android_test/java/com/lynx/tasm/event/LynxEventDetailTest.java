// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.event.LynxTouchEvent.Point;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;

public class LynxEventDetailTest {
  @Test
  public void customEventHasNoTargetPointMap() {
    LynxEventDetail detail = new LynxEventDetail(new LynxCustomEvent(1, "custom"), null, null);

    assertTrue(detail.getTargetPointMap().isEmpty());
  }

  @Test
  public void singleTouchEventHasNoTargetPointMap() {
    LynxEventDetail detail =
        new LynxEventDetail(new LynxTouchEvent(1, LynxTouchEvent.EVENT_TAP, 10, 20), null, null);

    assertTrue(detail.getTargetPointMap().isEmpty());
  }

  @Test
  public void multiTouchEventRetainsTouchIdentifiersAndCoordinates() {
    HashMap<Integer, Point> points = new HashMap<>();
    points.put(3, new Point(10, 20));
    points.put(7, new Point(30, 40));
    LynxEventDetail detail = new LynxEventDetail(
        new LynxTouchEvent(1, LynxTouchEvent.EVENT_TOUCH_MOVE, points), null, null);

    assertSame(points, detail.getTargetPointMap());
    assertEquals(new Point(10, 20), detail.getTargetPointMap().get(3));
    assertEquals(new Point(30, 40), detail.getTargetPointMap().get(7));
  }

  @Test
  public void multiTouchEventWithoutCoordinatesHasEmptyTargetPointMap() {
    LynxEventDetail detail = new LynxEventDetail(
        new LynxTouchEvent(1, LynxTouchEvent.EVENT_TOUCH_START, (Map<Integer, Point>) null), null,
        null);

    assertTrue(detail.getTargetPointMap().isEmpty());
  }

  @Test
  public void customEventReturnsPayloadWithoutEnvelope() {
    HashMap<String, Object> params = new HashMap<>();
    params.put("value", 42);
    LynxEventDetail detail =
        new LynxEventDetail(new LynxCustomEvent(1, "custom", params), null, null);

    assertSame(params, detail.getEventParams());
    assertEquals(42, detail.getEventParams().get("value"));
  }

  @Test
  public void detailEventReturnsPayloadWithoutEnvelope() {
    HashMap<String, Object> params = new HashMap<>();
    params.put("value", 42);
    LynxEventDetail detail =
        new LynxEventDetail(new LynxDetailEvent(1, "custom", params), null, null);

    assertSame(params, detail.getEventParams());
  }

  @Test
  public void customEventWithoutPayloadReturnsEmptyParameters() {
    LynxEventDetail detail =
        new LynxEventDetail(new LynxCustomEvent(1, "custom", null), null, null);

    assertTrue(detail.getEventParams().isEmpty());
  }
}
