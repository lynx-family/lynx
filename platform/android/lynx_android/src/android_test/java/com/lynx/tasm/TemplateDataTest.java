// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;
import static org.junit.Assert.assertEquals;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.base.LLog;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class TemplateDataTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }

  @Test
  public void empty() {
    TemplateData templateData = TemplateData.empty();
    assertEquals(templateData.getNativePtr(), 0);
  }

  @Test
  public void getNativePtr() {
    TemplateData templateData = TemplateData.empty();
    assertEquals(templateData.getNativePtr(), 0);
  }

  @Test
  public void markReadOnly() {
    TemplateData templateData = TemplateData.empty();
    assertFalse(templateData.isReadOnly());
    templateData.markReadOnly();
    assertTrue(templateData.isReadOnly());
  }

  @Test
  public void fromMap() {
    Map<String, Object> map = new HashMap<>();
    map.put("a", "1");
    TemplateData templateData = TemplateData.fromMap(map);
    assertNotEquals(templateData.getNativePtr(), 0);
    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertTrue(result.containsKey("a"));

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void toMap() {
    Map<String, Object> map = new HashMap<>();
    map.put("a", "1");
    TemplateData templateData = TemplateData.fromMap(map);
    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertEquals("1", result.get("a"));

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void testLong() {
    Map<String, Object> map = new HashMap<>();
    map.put("long", 123456789123456L);
    TemplateData templateData = TemplateData.fromMap(map);
    assertNotEquals(templateData.getNativePtr(), 0);
    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertEquals(123456789123456L, result.get("long"));

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void testFromString() {
    TemplateData templateData = TemplateData.fromString("{\n"
        + "    \"language\": \"English\",\n"
        + "    \"please-input-password\": \"Please input your password:\",\n"
        + "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
        + "    \"input-box\": \"input box\",\n"
        + "    \"input-name\": \"name\",\n"
        + "    \"input-age\": \"age\",\n"
        + "    \"submit\": \"Submit\",\n"
        + "    \"cancel\": \"Cancel\",\n"
        + "    \"mode-lang-day\": \"Day Mode\",\n"
        + "    \"mode-lang-night\": \"Night Mode\",\n"
        + "    \"icon-url\": \"http://1571726511672_788.jpg\"\n"
        + "}\n");
    assertNotEquals(templateData.getNativePtr(), 0);
    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertNotEquals(result.size(), 0);
    assertEquals(result.get("please-input-password"), "Please input your password:");

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void testMergeTemplateData() {
    Map<String, Object> map = new HashMap<>();
    map.put("long", 123456789123456L);
    TemplateData templateData = TemplateData.fromMap(map);
    TemplateData diff = TemplateData.fromString("{\n"
        + "    \"language\": \"English\",\n"
        + "    \"please-input-password\": \"Please input your password:\",\n"
        + "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
        + "    \"input-box\": \"input box\",\n"
        + "    \"input-name\": \"name\",\n"
        + "    \"input-age\": \"age\",\n"
        + "    \"submit\": \"Submit\",\n"
        + "    \"cancel\": \"Cancel\",\n"
        + "    \"mode-lang-day\": \"Day Mode\",\n"
        + "    \"mode-lang-night\": \"Night Mode\",\n"
        + "    \"icon-url\": \"http://1571726511672_788.jpg\"\n"
        + "}\n");
    templateData.updateWithTemplateData(diff);
    assertNotEquals(templateData.getNativePtr(), 0);
    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertNotEquals(result.size(), 0);
    assertEquals(result.get("please-input-password"), "Please input your password:");
    assertEquals(123456789123456L, result.get("long"));

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void testToMapIntDouble() {
    TemplateData data = TemplateData.fromMap(new HashMap<>());
    data.put("key1", 1);
    data.put("key3", 3.0);
    data.put("key2", 2);
    data.put("key4", 4.0);
    Map map = data.toMap();
    assertEquals(map.get("key1"), 1);
    assertEquals(map.get("key2"), 2);
    assertNotEquals(map.get("key3"), 3);
    assertNotEquals(map.get("key4"), 4);
    assertNotEquals(map.get("key1"), 1.0);
    assertNotEquals(map.get("key2"), 2.0);
    assertEquals(map.get("key3"), 3.0);
    assertEquals(map.get("key4"), 4.0);

    assertNotEquals(data.toMap(), TemplateData.nativeGetData(data.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(data));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(data.toMap(), TemplateData.nativeGetData(data.mJsNativeData));
  }

  @Test
  public void testUpdateActions0() {
    System.loadLibrary("lynx");
    TemplateData data = TemplateData.fromMap(new HashMap<String, Object>() {
      { put("key0", 1); }
    });
    data.put("key1", 1);
    data.put("key3", 3.0);
    data.put("key2", 2);
    data.put("key4", 4.0);
    data.removeKey("key4");
    data.flush();

    Object nativeData = data.toMap();
    Object jsData = TemplateData.nativeGetData(data.getDataForJSThread());

    assertEquals(nativeData, jsData);
  }

  @Test
  public void testUpdateActions1() {
    System.loadLibrary("lynx");
    TemplateData data0 = TemplateData.fromString("{\n"
        + "    \"language\": \"English\",\n"
        + "    \"please-input-password\": \"Please input your password:\",\n"
        + "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
        + "    \"input-box\": \"input box\",\n"
        + "    \"input-name\": \"name\",\n"
        + "    \"input-age\": \"age\",\n"
        + "    \"submit\": \"Submit\",\n"
        + "    \"cancel\": \"Cancel\",\n"
        + "    \"mode-lang-day\": \"Day Mode\",\n"
        + "    \"mode-lang-night\": \"Night Mode\",\n"
        + "    \"icon-url\": \"http://1571726511672_788.jpg\"\n"
        + "}\n");
    data0.put("key1", 1);
    data0.put("key3", 3.0);
    data0.put("key2", 2);
    data0.put("key4", 4.0);
    data0.removeKey("key4");
    data0.flush();

    TemplateData data1 = TemplateData.fromMap(new HashMap<String, Object>() {
      {}
    });
    data1.put("key1", 1);
    data1.put("key3", 3.0);
    data1.put("key2", 2);
    data1.put("key4", 4.0);
    data1.removeKey("key4");
    data1.removeKey("key2");
    data1.flush();

    data0.updateWithTemplateData(data1);
    data0.flush();

    Object nativeData = data0.toMap();
    Object jsData = TemplateData.nativeGetData(data0.getDataForJSThread());

    assertEquals(nativeData, jsData);

    TemplateData data2 = data0.deepClone();

    Object nativeData2 = data2.toMap();
    Object jsData2 = TemplateData.nativeGetData(data2.getDataForJSThread());
    assertEquals(nativeData2, jsData2);
    assertEquals(nativeData, nativeData2);
    assertEquals(jsData, jsData2);
  }

  @Test
  public void testUpdateActions2() {
    System.loadLibrary("lynx");
    TemplateData data = TemplateData.empty();
    data.put("key1", 1);
    data.put("key3", 3.0);
    data.put("key2", 2);
    data.put("key4", 4.0);
    data.removeKey("key4");
    data.flush();
    data.put("key4", 1);
    data.put("key5", 3.0);
    data.put("key6", 2);
    data.put("key7", 4.0);
    data.flush();

    Object nativeData = data.toMap();
    Object jsData = TemplateData.nativeGetData(data.getDataForJSThread());

    assertEquals(nativeData, jsData);
  }

  @Test
  public void testSingletonMap() {
    Map map = Collections.singletonMap("a", "1");
    TemplateData templateData = TemplateData.fromMap(map);
    templateData.put("b", "2");
    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertEquals("1", result.get("a"));
    assertEquals("2", result.get("b"));

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void testFromEmptyString() {
    // Init with empty string.
    TemplateData templateData = TemplateData.fromString("");

    // Update map.
    Map map = Collections.singletonMap("a", "1");
    templateData.updateData(map);

    // Update with specific key.
    templateData.put("b", "2");

    // Update with another TemplateData, which init from empty string.
    templateData.updateWithTemplateData(TemplateData.fromString(""));

    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertEquals("1", result.get("a"));
    assertEquals("2", result.get("b"));

    templateData.flush();

    Object nativeData = templateData.toMap();
    Object jsData = TemplateData.nativeGetData(templateData.getDataForJSThread());

    assertEquals(nativeData, jsData);
  }

  @Test
  public void testFromInvalidString() {
    // Init with invalid string.
    TemplateData templateData = TemplateData.fromString("invalid json string!!!");

    // Update map.
    Map map = Collections.singletonMap("a", "1");
    templateData.updateData(map);

    // Update with specific key.
    templateData.put("b", "2");

    // Update with another TemplateData, which init from json array string.
    templateData.updateWithTemplateData(TemplateData.fromString("[]"));

    Map<Object, Object> result = templateData.toMap();
    assertNotNull(result);
    assertEquals("1", result.get("a"));
    assertEquals("2", result.get("b"));

    templateData.flush();

    Object nativeData = templateData.toMap();
    Object jsData = TemplateData.nativeGetData(templateData.getDataForJSThread());

    assertEquals(nativeData, jsData);
  }

  @Test
  public void testUpdateData() {
    // construct a valid templateData structure;
    TemplateData templateData = TemplateData.empty();
    templateData.put("key1", 1);
    templateData.flush();

    Map map = new HashMap();
    map.put("a", 1);
    map.put("b", 2);
    templateData.put("key2", map);

    Map map2 = new HashMap();
    map2.put("a", 1);
    map2.put("b", 3);
    templateData.put("key2", map2);
    templateData.flush();

    Map nativeData = templateData.toMap();
    assertEquals(nativeData.get("key1"), 1);
    assertTrue(nativeData.get("key2") instanceof Map);

    Map key2Map = (Map) nativeData.get("key2");
    assertEquals(key2Map.get("a"), 1);
    assertEquals(key2Map.get("b"), 3);

    assertNotEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData.toMap(), TemplateData.nativeGetData(templateData.mJsNativeData));
  }

  @Test
  public void testConsumeUpdateActions0() {
    TemplateData templateData1 = TemplateData.empty();
    templateData1.put("key1", 1);
    templateData1.flush();

    TemplateData templateData2 = TemplateData.empty();
    templateData2.put("key2", 2);
    assertEquals(0, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.flush();
    assertEquals(1, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.put("key3", 3);
    assertEquals(1, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.flush();
    assertEquals(2, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.put("key4", 4);
    assertEquals(2, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.flush();
    assertEquals(3, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.put("key5", 5);
    assertEquals(3, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData1.updateWithTemplateData(templateData2);

    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(0, templateData2.mUpdateActions.size());
    assertNotEquals(0, templateData2.mJsNativeData);

    TemplateData templateData3 = TemplateData.empty();
    templateData3.put("key1", 1);
    templateData3.updateWithTemplateData(templateData2);

    assertEquals(templateData1.toMap(), templateData3.toMap());
    assertEquals(
        templateData1.toMap(), TemplateData.nativeGetData(templateData3.getDataForJSThread()));
  }

  @Test
  public void testConsumeUpdateActions1() {
    TemplateData templateData2 = TemplateData.empty();
    templateData2.put("key2", 2);
    assertEquals(0, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.flush();
    assertEquals(1, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.put("key3", 3);
    assertEquals(1, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.flush();
    assertEquals(2, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.put("key4", 4);
    assertEquals(2, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.flush();
    assertEquals(3, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.put("key5", 5);
    assertEquals(3, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    templateData2.updateWithTemplateData(templateData2);

    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(3, templateData2.mUpdateActions.size());
    assertEquals(0, templateData2.mJsNativeData);

    assertNotEquals(templateData2.toMap(), TemplateData.nativeGetData(templateData2.mJsNativeData));
    TemplateData.releaseNativeTemplateData(TemplateData.createNativeTemplateData(templateData2));
    System.gc();
    try {
      Thread.sleep(3000);
    } catch (Throwable e) {
      LLog.e("TemplateDataTest", e.toString());
    }

    assertEquals(templateData2.toMap(), TemplateData.nativeGetData(templateData2.mJsNativeData));
  }

  public void testNativeMerge() {
    TemplateData templateData1 = TemplateData.empty();
    TemplateData templateData2 = TemplateData.empty();

    templateData1.updateWithTemplateData(templateData2);
    TemplateData.nativeMergeTemplateData(
        templateData1.getNativePtr(), templateData2.getNativePtr());

    templateData1.put("key1", 1);
    templateData1.flush();
    TemplateData.nativeMergeTemplateData(
        templateData1.getNativePtr(), templateData2.getNativePtr());

    templateData2.put("key2", 2);
    TemplateData.nativeMergeTemplateData(
        templateData1.getNativePtr(), templateData2.getNativePtr());

    Map map = new HashMap();
    map.put("key1", 1);
    map.put("key2", 2);

    assertEquals(TemplateData.nativeGetData(templateData1.getNativePtr()), map);
  }

  @Test
  public void testJsonArrayToList() {
    try {
      String jsonArrayString = "[{\"name\":\"Alice\"}, {\"name\":\"Bob\"}]";
      JSONArray jsonArray = new JSONArray(jsonArrayString);

      List<Object> result = TemplateData.jsonArrayToList(jsonArray);

      assertEquals(2, result.size());
      assertTrue(result.get(0) instanceof Map);
      assertTrue(result.get(1) instanceof Map);

      Map<String, Object> firstObject = (Map<String, Object>) result.get(0);
      assertEquals("Alice", firstObject.get("name"));
    } catch (Throwable e) {
      fail(e.getMessage());
    }
  }

  @Test
  public void testJsonToMap() {
    try {
      String jsonString = "{\"name\":\"John\", \"age\":30, \"skills\":[\"Java\",\"Kotlin\"]}";
      JSONObject jsonObject = new JSONObject(jsonString);

      Map<String, Object> result = TemplateData.jsonToMap(jsonObject);

      assertEquals(3, result.size());
      assertEquals("John", result.get("name"));
      assertEquals(30, result.get("age"));

      List<String> skills = (List<String>) result.get("skills");
      assertEquals(2, skills.size());
      assertTrue(skills.contains("Java"));
      assertTrue(skills.contains("Kotlin"));
    } catch (Throwable e) {
      fail(e.getMessage());
    }
  }

  @Test
  public void testByteArray() {
    Map map = new HashMap();
    map.put("key1", "value1");
    map.put("key2", "value2".getBytes());
    TemplateData data = TemplateData.fromMap(map);
    Map testMap = data.toMap();
    assertEquals(map.get("key1"), testMap.get("key1"));

    byte[] bytes = ((byte[]) map.get("key2"));
    byte[] testBytes = ((byte[]) testMap.get("key2"));
    assertTrue(bytes.length == testBytes.length);
    assertTrue(Arrays.equals(bytes, testBytes));
  }

  @Test
  public void testNestedJson() {
    try {
      String nestedJsonString =
          "{\"name\":\"John\",\"details\":{\"age\":30,\"skills\":[\"Java\",\"Python\"]}}";
      JSONObject jsonObject = new JSONObject(nestedJsonString);

      Map<String, Object> result = TemplateData.jsonToMap(jsonObject);

      assertEquals("John", result.get("name"));

      Map<String, Object> details = (Map<String, Object>) result.get("details");
      assertEquals(30, details.get("age"));

      List<String> skills = (List<String>) details.get("skills");
      assertEquals(2, skills.size());
      assertTrue(skills.contains("Java"));
      assertTrue(skills.contains("Python"));
    } catch (Throwable e) {
      fail(e.getMessage());
    }
  }

  @Test
  public void testGetTemplateDataForJSThread() {
    TemplateData templateData = TemplateData.fromMap(new HashMap<>());
    templateData.put("key1", 1);
    templateData.put("key2", 2);
    templateData.put("key3", 3);
    templateData.put("key4", 4);
    templateData.flush();

    TemplateData copiedJsTemplateData = templateData.getTemplateDataForJSThread();
    Object copiedJsData = TemplateData.nativeGetData(copiedJsTemplateData.getDataForJSThread());

    Object jsData = TemplateData.nativeGetData(templateData.getDataForJSThread());
    assertEquals(copiedJsData, jsData);

    templateData.put("key5", 5);
    templateData.flush();
    Object updatedJsData = TemplateData.nativeGetData(templateData.getDataForJSThread());
    assertEquals(updatedJsData, templateData.toMap());
  }

  @Test
  public void testDeepCloneBeforeGetTemplateDataForJSThread() {
    TemplateData templateData = TemplateData.fromMap(new HashMap<>());
    templateData.put("key1", 1);
    templateData.put("key2", 2);
    templateData.put("key3", 3);
    templateData.put("key4", 4);
    templateData.flush();
    TemplateData clonedData = templateData.deepClone();

    TemplateData copiedJsTemplateData = templateData.getTemplateDataForJSThread();
    Object copiedJsData = TemplateData.nativeGetData(copiedJsTemplateData.getDataForJSThread());
    assertEquals(copiedJsData, clonedData.toMap());
  }

  @Test
  public void testRemoveData() {
    TemplateData templateData = TemplateData.fromMap(new HashMap<>());
    templateData.put("key1", 1);
    templateData.put("key2", 2);
    templateData.put("key3", 3);
    templateData.put("key4", 4);
    templateData.flush();
    templateData.remove("key4");

    Map<Object, Object> map = templateData.toMap();
    assertEquals(map.size(), 3);
    assertEquals(map.get("key1"), 1);
    assertEquals(map.get("key2"), 2);
    assertEquals(map.get("key3"), 3);
    assertFalse(map.containsKey("key4"));
  }

  @Test
  public void testConcurrentPut() throws Exception {
    TemplateData data = TemplateData.empty();
    data.markConcurrent();

    int threadCount = 10;
    int perThreadOps = 1000;
    ExecutorService pool = Executors.newFixedThreadPool(threadCount);

    CountDownLatch latch = new CountDownLatch(threadCount);

    for (int t = 0; t < threadCount; t++) {
      final int threadId = t;
      pool.submit(() -> {
        for (int i = 0; i < perThreadOps; i++) {
          data.put("key-" + threadId + "-" + i, i);
        }
        latch.countDown();
      });
    }

    latch.await();
    Map<Object, Object> map = data.toMap();

    for (int t = 0; t < threadCount; t++) {
      for (int i = 0; i < perThreadOps; i++) {
        assertEquals(i, map.get("key-" + t + "-" + i));
      }
    }
  }
}
