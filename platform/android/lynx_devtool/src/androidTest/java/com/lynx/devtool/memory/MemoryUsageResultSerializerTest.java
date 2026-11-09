// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.memory;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxGlobalMemoryUsageResult;
import com.lynx.tasm.LynxInstanceMemoryUsage;
import com.lynx.tasm.LynxMemoryCollectionStatus;
import com.lynx.tasm.performance.memory.MemoryRecord;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.json.JSONObject;
import org.junit.Test;

public class MemoryUsageResultSerializerTest {
  @Test
  public void serializesEmptyResult() throws Exception {
    LynxGlobalMemoryUsageResult result = createResult(
        LynxMemoryCollectionStatus.COMPLETED, 0, 0, new ArrayList<LynxInstanceMemoryUsage>());

    JSONObject json = MemoryUsageResultSerializer.toJson(result);

    assertEquals("completed", json.getString("collectionStatus"));
    assertEquals(0, json.getInt("expectedInstanceCount"));
    assertEquals(0, json.getInt("completedInstanceCount"));
    assertEquals(0L, json.getLong("totalBytes"));
    assertEquals(0, json.getJSONArray("instances").length());
  }

  @Test
  public void serializesInstanceAndViewDetail() throws Exception {
    Map<String, String> detail = new HashMap<>();
    detail.put(null, "ignored");
    detail.put("", "ignored");
    detail.put("source", "test");
    MemoryRecord viewRecord = new MemoryRecord("view", 40L, 2, detail);
    Map<String, MemoryRecord> viewDetail = new HashMap<>();
    viewDetail.put(null, viewRecord);
    viewDetail.put("", viewRecord);
    viewDetail.put("view", viewRecord);
    ArrayList<LynxInstanceMemoryUsage> instances = new ArrayList<>();
    instances.add(createInstance(1, null, null, 10L, 2L, 40L, viewDetail, 30L, 20L, null));
    LynxGlobalMemoryUsageResult result =
        createResult(LynxMemoryCollectionStatus.TIMEOUT, 1, 4096L, instances);

    JSONObject json = MemoryUsageResultSerializer.toJson(result);

    assertEquals("timeout", json.getString("collectionStatus"));
    assertEquals(100L, json.getLong("totalBytes"));
    assertEquals(4096L, json.getLong("appBytes"));
    JSONObject instance = json.getJSONArray("instances").getJSONObject(0);
    assertEquals("", instance.getString("pageId"));
    assertEquals("", instance.getString("url"));
    assertEquals("", instance.getString("btsRuntimeGroupId"));
    JSONObject record = instance.getJSONObject("viewDetail").getJSONObject("view");
    assertFalse(instance.getJSONObject("viewDetail").has(""));
    assertEquals("view", record.getString("category"));
    assertEquals(40L, record.getLong("sizeBytes"));
    assertEquals(2, record.getInt("instanceCount"));
    assertFalse(record.getJSONObject("detail").has(""));
    assertEquals("test", record.getJSONObject("detail").getString("source"));
  }

  @Test
  public void serializesEmptyInstanceListWithExpectedCount() throws Exception {
    LynxGlobalMemoryUsageResult result = createResult(
        LynxMemoryCollectionStatus.COMPLETED, 2, 0L, new ArrayList<LynxInstanceMemoryUsage>());

    JSONObject json = MemoryUsageResultSerializer.toJson(result);

    assertEquals("completed", json.getString("collectionStatus"));
    assertEquals(2, json.getInt("expectedInstanceCount"));
    assertEquals(0, json.getInt("completedInstanceCount"));
    assertEquals(0, json.getJSONArray("instances").length());
  }

  @Test
  public void queryAllMemoryUsageInvokesCallbackOnce() throws Exception {
    LynxEnv env = LynxEnv.inst();
    boolean wasNativeLibraryLoaded = env.isNativeLibraryLoaded();
    env.setNativeLibraryLoaded(false);
    CountDownLatch latch = new CountDownLatch(1);
    final int[] callbackCount = {0};

    try {
      MemoryUsageResultSerializer.queryAllMemoryUsage(
          1L, new MemoryUsageResultSerializer.ResultCallback() {
            @Override
            public void onResult(String resultJson, String errorMessage) {
              callbackCount[0]++;
              try {
                JSONObject json = new JSONObject(resultJson);
                assertEquals("completed", json.getString("collectionStatus"));
                assertEquals(0, json.getInt("expectedInstanceCount"));
                assertTrue(errorMessage.isEmpty());
              } catch (Exception exception) {
                throw new AssertionError(exception);
              }
              latch.countDown();
            }
          });

      assertTrue(latch.await(10, TimeUnit.SECONDS));
      assertEquals(1, callbackCount[0]);
    } finally {
      env.setNativeLibraryLoaded(wasNativeLibraryLoaded);
    }
  }

  private static LynxInstanceMemoryUsage createInstance(int instanceId, String pageId, String url,
      long elementBytes, long elementNodeCount, long viewBytes,
      Map<String, MemoryRecord> viewDetail, long mainThreadRuntimeBytes,
      long backgroundThreadRuntimeBytes, String btsRuntimeGroupId) throws Exception {
    Constructor<LynxInstanceMemoryUsage> constructor =
        LynxInstanceMemoryUsage.class.getDeclaredConstructor(int.class, String.class, String.class,
            long.class, long.class, long.class, long.class, Map.class, long.class, long.class,
            String.class);
    constructor.setAccessible(true);
    long totalBytes =
        elementBytes + viewBytes + mainThreadRuntimeBytes + backgroundThreadRuntimeBytes;
    return constructor.newInstance(instanceId, pageId, url, totalBytes, elementBytes,
        elementNodeCount, viewBytes, viewDetail, mainThreadRuntimeBytes,
        backgroundThreadRuntimeBytes, btsRuntimeGroupId);
  }

  private static LynxGlobalMemoryUsageResult createResult(LynxMemoryCollectionStatus status,
      int expectedInstanceCount, long appBytes, ArrayList<LynxInstanceMemoryUsage> instances)
      throws Exception {
    Method build = LynxGlobalMemoryUsageResult.class.getDeclaredMethod("build", long.class,
        LynxMemoryCollectionStatus.class, long.class, long.class, int.class, long.class,
        java.util.List.class);
    build.setAccessible(true);
    return (LynxGlobalMemoryUsageResult) build.invoke(
        null, 100L, status, 20L, 2000L, expectedInstanceCount, appBytes, instances);
  }
}
