// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.common;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.TemplateData;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LepusBufferTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }

  @Test
  public void javaEncodeAndCppDecodeStayAlignedForIntLongDouble() {
    Map<String, Object> source = new LinkedHashMap<>();
    source.put("int_val", 42);
    source.put("long_val", 1L << 40);
    source.put("double_val", 6.25d);
    source.put("tail_string", "after_double");

    TemplateData templateData = TemplateData.fromMap(source);
    Map<Object, Object> result = templateData.toMap();

    assertNotNull(result);
    assertEquals(42, result.get("int_val"));
    assertEquals(1L << 40, result.get("long_val"));
    assertEquals(6.25d, (Double) result.get("double_val"), 0.0d);
    assertEquals("after_double", result.get("tail_string"));
  }

  @Test
  public void javaEncodeAndCppDecodeStayAlignedForNestedCollection() {
    List<Object> list = new ArrayList<>();
    list.add(7);
    list.add("hello");
    list.add(Boolean.FALSE);

    Map<String, Object> nestedMap = new LinkedHashMap<>();
    nestedMap.put("nested_long", 1L << 40);
    nestedMap.put("nested_double", 3.5d);

    Map<String, Object> source = new LinkedHashMap<>();
    source.put("list_val", list);
    source.put("dict_val", nestedMap);

    TemplateData templateData = TemplateData.fromMap(source);
    Map<Object, Object> result = templateData.toMap();

    assertNotNull(result);
    List<?> resultList = (List<?>) result.get("list_val");
    assertNotNull(resultList);
    assertEquals(7, resultList.get(0));
    assertEquals("hello", resultList.get(1));
    assertEquals(Boolean.FALSE, resultList.get(2));

    Map<?, ?> resultMap = (Map<?, ?>) result.get("dict_val");
    assertNotNull(resultMap);
    assertEquals(1L << 40, resultMap.get("nested_long"));
    assertEquals(3.5d, (Double) resultMap.get("nested_double"), 0.0d);
  }

  @Test
  public void cppEncodeAndJavaDecodeStayAlignedAfterNativeMutation() {
    TemplateData templateData = TemplateData.empty();
    templateData.put("int_val", 42);
    templateData.put("long_val", 1L << 40);
    templateData.put("double_val", 6.25d);
    templateData.put("tail_string", "after_double");

    Map<Object, Object> result = templateData.toMap();

    assertNotNull(result);
    assertEquals(42, result.get("int_val"));
    assertEquals(1L << 40, result.get("long_val"));
    assertEquals(6.25d, (Double) result.get("double_val"), 0.0d);
    assertEquals("after_double", result.get("tail_string"));
  }
}
