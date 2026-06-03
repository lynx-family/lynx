// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.performance.memory.MemoryRecord;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class LynxTemplateRenderMemoryUsageTest {
  @Test
  public void contextWaitsForNativeAndViewSnapshots() throws Exception {
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicReference<LynxInstanceMemoryUsage> resultRef = new AtomicReference<>();
    LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery query =
        createContext(7, "lynx://page", result -> {
          callbackCount.incrementAndGet();
          resultRef.set(result);
        });

    completeNativeSnapshot(query, 10L, 2L, 30L, 40L, "group-a");
    assertEquals(0, callbackCount.get());

    completeViewBytes(query, 50L, createViewDetail("image", 50L, 2));

    assertEquals(1, callbackCount.get());
    LynxInstanceMemoryUsage result = resultRef.get();
    assertNotNull(result);
    assertEquals(7, result.getInstanceId());
    assertNull(result.getPageId());
    assertEquals("lynx://page", result.getUrl());
    assertEquals(130L, result.getTotalBytes());
    assertEquals(10L, result.getElementBytes());
    assertEquals(2L, result.getElementNodeCount());
    assertEquals(50L, result.getViewBytes());
    MemoryRecord imageDetail = result.getViewDetail().get("image");
    assertNotNull(imageDetail);
    assertEquals("image", imageDetail.getCategory());
    assertEquals(50L, imageDetail.mSizeBytes);
    assertEquals(2, imageDetail.mInstanceCount);
    assertEquals(30L, result.getMainThreadRuntimeBytes());
    assertEquals(40L, result.getBackgroundThreadRuntimeBytes());
    assertEquals("group-a", result.getBtsRuntimeGroupId());

    completeNativeSnapshot(query, 1L, 1L, 1L, 1L, "group-b");
    completeViewBytes(query, 1L, null);
    assertEquals(1, callbackCount.get());
  }

  @Test
  public void contextCompletesWhenViewSnapshotArrivesFirst() throws Exception {
    AtomicReference<LynxInstanceMemoryUsage> resultRef = new AtomicReference<>();
    LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery query =
        createContext(8, "lynx://view-first", resultRef::set);

    completeViewBytes(query, 11L, null);
    assertNull(resultRef.get());

    completeNativeSnapshot(query, 13L, 3L, 17L, 19L, null);

    LynxInstanceMemoryUsage result = resultRef.get();
    assertNotNull(result);
    assertEquals(60L, result.getTotalBytes());
    assertEquals(11L, result.getViewBytes());
    assertTrue(result.getViewDetail().isEmpty());
    assertNull(result.getBtsRuntimeGroupId());
  }

  @Test
  public void contextCopiesViewDetailBeforePublishingResult() throws Exception {
    AtomicReference<LynxInstanceMemoryUsage> resultRef = new AtomicReference<>();
    LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery query =
        createContext(9, "lynx://copy-view-detail", resultRef::set);
    HashMap<String, String> imageExtra = new HashMap<>();
    imageExtra.put("url", "image-a");
    MemoryRecord uiOwnedImageRecord = new MemoryRecord("image", 50L, 2, imageExtra);
    HashMap<String, MemoryRecord> viewDetail = new HashMap<>();
    viewDetail.put("image", uiOwnedImageRecord.copy());

    completeViewBytes(query, 50L, viewDetail);
    completeNativeSnapshot(query, 10L, 1L, 0L, 0L, null);

    uiOwnedImageRecord.mSizeBytes = 99L;
    imageExtra.put("url", "image-b");
    viewDetail.put("image", new MemoryRecord("image", 99L, 3, null));

    LynxInstanceMemoryUsage result = resultRef.get();
    assertNotNull(result);
    MemoryRecord imageDetail = result.getViewDetail().get("image");
    assertNotNull(imageDetail);
    assertEquals(50L, imageDetail.mSizeBytes);
    assertEquals(2, imageDetail.mInstanceCount);
    assertEquals("image-a", imageDetail.mDetail.get("url"));
  }

  @Test
  public void sampleViewMemoryUsageCopiesDetachedRecords() throws Exception {
    HashMap<String, String> imageExtra = new HashMap<>();
    imageExtra.put("url", "image-a");
    MemoryRecord imageRecord = new MemoryRecord("image", 50L, 2, imageExtra);
    MemoryRecord textRecord = new MemoryRecord("text", 12L, 3, null);
    MemoryRecord unnamedRecord = new MemoryRecord("", 7L, 1, null);
    HashMap<String, MemoryRecord> records = new HashMap<>();
    records.put("image", imageRecord);
    records.put("", textRecord);
    records.put("unnamed", unnamedRecord);
    records.put("missing", null);

    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    when(uiOwner.getMemoryUsage()).thenReturn(records);

    HashMap<String, MemoryRecord> viewDetail = new HashMap<>();
    long viewBytes = sampleViewMemoryUsage(uiOwner, viewDetail);

    imageRecord.mSizeBytes = 99L;
    imageExtra.put("url", "image-b");

    assertEquals(69L, viewBytes);
    assertEquals(3, viewDetail.size());
    MemoryRecord copiedImage = viewDetail.get("image");
    assertNotNull(copiedImage);
    assertEquals(50L, copiedImage.mSizeBytes);
    assertEquals("image-a", copiedImage.mDetail.get("url"));
    assertNotNull(viewDetail.get("text"));
    assertNotNull(viewDetail.get("unnamed"));
  }

  @Test
  public void sampleViewMemoryUsageReturnsEmptyForMissingOwner() throws Exception {
    HashMap<String, MemoryRecord> viewDetail = new HashMap<>();

    long viewBytes = sampleViewMemoryUsage(null, viewDetail);

    assertEquals(0L, viewBytes);
    assertTrue(viewDetail.isEmpty());
  }

  @Test
  public void fetcherReturnsNullWhenRenderIsGone() throws Exception {
    LynxMemoryUsageFetcher fetcher = new LynxTemplateRenderMemoryUsageFetcher(null);
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicReference<LynxInstanceMemoryUsage> resultRef = new AtomicReference<>();

    fetcher.queryMemoryUsageAsync(result -> {
      callbackCount.incrementAndGet();
      resultRef.set(result);
    });

    assertEquals(1, callbackCount.get());
    assertNull(resultRef.get());
  }

  private static LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery createContext(
      int instanceId, String url, LynxConsumer<LynxInstanceMemoryUsage> callback) {
    return new LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery(
        instanceId, url, callback);
  }

  private static void completeNativeSnapshot(
      LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery query, long elementBytes,
      long elementNodeCount, long mainThreadRuntimeBytes, long backgroundThreadRuntimeBytes,
      String btsRuntimeGroupId) {
    query.completeNativeSnapshot(elementBytes, elementNodeCount, mainThreadRuntimeBytes,
        backgroundThreadRuntimeBytes, btsRuntimeGroupId);
  }

  private static void completeViewBytes(
      LynxTemplateRenderMemoryUsageFetcher.InstanceMemoryUsageQuery query, long viewBytes,
      Map<String, MemoryRecord> viewDetail) {
    query.completeViewSnapshot(viewBytes, viewDetail);
  }

  private static long sampleViewMemoryUsage(
      LynxUIOwner uiOwner, HashMap<String, MemoryRecord> viewDetail) throws Exception {
    Method method = LynxTemplateRenderMemoryUsageFetcher.class.getDeclaredMethod(
        "sampleViewMemoryUsage", LynxUIOwner.class, HashMap.class);
    method.setAccessible(true);
    return ((Long) method.invoke(null, uiOwner, viewDetail)).longValue();
  }

  private static HashMap<String, MemoryRecord> createViewDetail(
      String category, long sizeBytes, int instanceCount) {
    HashMap<String, MemoryRecord> viewDetail = new HashMap<>();
    viewDetail.put(category, new MemoryRecord(category, sizeBytes, instanceCount, null).copy());
    return viewDetail;
  }
}
