// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.*;

import org.junit.Test;

public class ScreencastAckTest {
  @Test
  public void onlyMatchingAckCommitsReference() {
    ScreencastAck ack = new ScreencastAck();
    assertTrue(ack.needsFullFrame());
    int id = ack.send(100, false);
    assertFalse(ack.acknowledge(0, true));
    assertFalse(ack.acknowledge(id + 1, true));
    assertTrue(ack.isPending());
    assertEquals(0, ack.reference());
    assertTrue(ack.acknowledge(id, true));
    assertFalse(ack.acknowledge(id, true));
    assertEquals(id, ack.reference());
    assertFalse(ack.needsFullFrame());
  }

  @Test(expected = IllegalStateException.class)
  public void cannotQueueFramesBehindUnacknowledgedFrame() {
    ScreencastAck ack = new ScreencastAck();
    ack.send(0, false);
    ack.send(1, false);
  }

  @Test
  public void timeoutDiscardsReferenceAndRejectsLateAck() {
    ScreencastAck ack = new ScreencastAck();
    int id = ack.send(100, false);
    assertFalse(ack.expire(2099));
    assertTrue(ack.expire(2100));
    int next = ack.send(2100, false);
    assertFalse(ack.acknowledge(id, true));
    assertTrue(ack.needsFullFrame());
    assertTrue(ack.acknowledge(next, true));
  }

  @Test
  public void rejectedPatchRequestsFullFrame() {
    ScreencastAck ack = new ScreencastAck();
    ack.acknowledge(ack.send(0, false), true);
    assertTrue(ack.acknowledge(ack.send(200, true), false));
    assertTrue(ack.needsFullFrame());
    assertEquals(0, ack.reference());
  }

  @Test
  public void periodicFullFrameBoundsPatchChain() {
    ScreencastAck ack = new ScreencastAck();
    ack.acknowledge(ack.send(0, false), true);
    for (int i = 0; i < 60; i++) {
      assertFalse(ack.needsFullFrame());
      ack.acknowledge(ack.send(i, true), true);
    }
    assertTrue(ack.needsFullFrame());
    ack.acknowledge(ack.send(100, false), true);
    assertFalse(ack.needsFullFrame());
  }

  @Test
  public void restartedSessionRejectsOldFrame() {
    ScreencastAck old = new ScreencastAck();
    int oldId = old.send(0, false);
    ScreencastAck next = new ScreencastAck();
    int nextId = next.send(0, false);
    assertNotEquals(oldId, nextId);
    assertFalse(next.acknowledge(oldId, true));
    next.reset();
    assertFalse(next.acknowledge(nextId, true));
    assertTrue(next.needsFullFrame());
  }
}
