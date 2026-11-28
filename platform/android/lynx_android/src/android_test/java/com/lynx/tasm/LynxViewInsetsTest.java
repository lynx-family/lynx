// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import org.junit.Test;

public class LynxViewInsetsTest {
  @Test
  public void partialUpdatePreservesOtherCategoriesAndReplacesWholeState() {
    LynxViewInsets.State nav =
        new LynxViewInsets.State(new LynxViewInsets.Edges(0, 0, 0, 24), true);
    LynxViewInsets initial =
        new LynxViewInsets.Builder().setInset(LynxViewInsets.Type.NAVIGATION_BARS, nav).build();
    LynxViewInsets.State ime =
        new LynxViewInsets.State(new LynxViewInsets.Edges(0, 0, 0, 300), true);
    LynxViewInsets next = initial.merge(new LynxViewInsets.Builder()
                                            .setInset(LynxViewInsets.Type.IME, ime)
                                            .setInset(LynxViewInsets.Type.NAVIGATION_BARS, null)
                                            .build());
    assertSame(nav, next.categories.get(LynxViewInsets.Type.NAVIGATION_BARS));
    assertFalse(initial.categories.containsKey(LynxViewInsets.Type.IME));
    assertEquals(next.toMap(), next.merge(LynxViewInsets.empty()).toMap());
    LynxViewInsets.State hidden =
        new LynxViewInsets.State(new LynxViewInsets.Edges(0, 0, 0, 0), false);
    LynxViewInsets cleared = next.merge(
        new LynxViewInsets.Builder().setInset(LynxViewInsets.Type.NAVIGATION_BARS, hidden).build());
    assertFalse(cleared.categories.get(LynxViewInsets.Type.NAVIGATION_BARS).visible);
    assertSame(ime, cleared.categories.get(LynxViewInsets.Type.IME));
  }

  @Test
  public void geometryAndVisibilityAreIndependent() {
    LynxViewInsets.State visibleZero =
        new LynxViewInsets.State(new LynxViewInsets.Edges(0, 0, 0, 0), true);
    LynxViewInsets update =
        new LynxViewInsets.Builder().setInset(LynxViewInsets.Type.IME, visibleZero).build();
    assertEquals(Boolean.TRUE, update.categories.get(LynxViewInsets.Type.IME).visible);
    assertEquals(0f, ((LynxViewInsets.Edges) visibleZero.geometry).bottom, 0f);
  }

  @SuppressWarnings("unchecked")
  @Test
  public void snapshotsDetachCollectionsAndPreserveSignedCoordinates() {
    List<LynxViewInsets.Rect> rects = new ArrayList<>();
    rects.add(new LynxViewInsets.Rect(-10, -20, 30, 40));
    LynxViewInsets.Builder builder =
        new LynxViewInsets.Builder().setInset(LynxViewInsets.Type.DISPLAY_CUTOUT,
            new LynxViewInsets.State(new LynxViewInsets.Rects(rects), null));
    LynxViewInsets first = builder.build();
    rects.clear();
    builder.setInset(LynxViewInsets.Type.DISPLAY_CUTOUT, null);
    Map<String, Object> state = (Map<String, Object>) first.toMap().get("displayCutout");
    Map<String, Object> geometry = (Map<String, Object>) state.get("geometry");
    assertEquals("rects", geometry.get("type"));
    List<Map<String, Object>> values = (List<Map<String, Object>>) geometry.get("rects");
    assertEquals(-10f, values.get(0).get("x"));
    assertEquals(1, values.size());
    assertTrue(builder.build().categories.isEmpty());
    try {
      first.categories.clear();
      fail("Snapshot categories must be immutable");
    } catch (UnsupportedOperationException expected) {
    }
    try {
      values.clear();
      fail("Serialized lists must be immutable");
    } catch (UnsupportedOperationException expected) {
    }
    LynxViewInsets.Corner corner = new LynxViewInsets.Corner(-20, 10, 30);
    assertEquals(-20f, corner.centerX, 0f);
  }

  @Test
  public void rejectsInvalidGeometryAndCategoryWithoutChangingBuilder() {
    LynxViewInsets.Builder builder = new LynxViewInsets.Builder();
    LynxViewInsets.State valid =
        new LynxViewInsets.State(new LynxViewInsets.Edges(0, 0, 0, 24), true);
    builder.setInset(LynxViewInsets.Type.IME, valid);
    try {
      builder.setInset(LynxViewInsets.Type.IME,
          new LynxViewInsets.State(new LynxViewInsets.Rects(Collections.emptyList()), true));
      fail("IME must use edges");
    } catch (IllegalArgumentException expected) {
    }
    assertSame(valid, builder.build().categories.get(LynxViewInsets.Type.IME));
  }

  @Test(expected = IllegalArgumentException.class)
  public void rejectsNegativeEdges() {
    new LynxViewInsets.Edges(0, -1, 0, 0);
  }

  @Test(expected = IllegalArgumentException.class)
  public void rejectsNonFiniteCoordinates() {
    new LynxViewInsets.Rect(Float.NaN, 0, 1, 1);
  }

  @Test(expected = IllegalArgumentException.class)
  public void rejectsNegativeRadius() {
    new LynxViewInsets.Corner(0, 0, -1);
  }
}
