// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.animax.setting;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Collections;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LynxAnimaXSettingServiceTest {
  private LynxAnimaXSettingService settingService;

  @Mock private ILynxTrailService trailService;
  @Mock private LynxServiceCenter mockServiceCenter;

  private LynxServiceCenter originalServiceCenter;

  @Before
  public void setUp() throws Exception {
    MockitoAnnotations.openMocks(this);
    settingService = new LynxAnimaXSettingService();

    originalServiceCenter = LynxServiceCenter.inst();
    injectMockServiceCenter();

    when(mockServiceCenter.getService(ILynxTrailService.class)).thenReturn(trailService);
  }

  @After
  public void tearDown() throws Exception {
    setServiceCenterInstance(originalServiceCenter);
    settingService.clearCache();
  }

  private void injectMockServiceCenter() throws Exception {
    setServiceCenterInstance(mockServiceCenter);
  }

  private void setServiceCenterInstance(LynxServiceCenter instance) throws Exception {
    Field instanceField = LynxServiceCenter.class.getDeclaredField("instance");
    instanceField.setAccessible(true);
    instanceField.set(null, instance);
    instanceField.setAccessible(false);
  }

  @Test
  public void testGetValueByKey_TrailServiceNotAvailable() {
    // Arrange
    when(mockServiceCenter.getService(ILynxTrailService.class)).thenReturn(null);

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue(
        "Should return empty string value when trail service is not available", result.isString());
    assertEquals("", result.getStringOrEmpty());
  }

  @Test
  public void testGetValueByKey_NullValue() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key")).thenReturn(null);

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return empty string value for null trail value", result.isString());
    assertEquals("", result.getStringOrEmpty());
  }

  @Test
  public void testGetValueByKey_StringValue() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key")).thenReturn("test_value");

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return string value", result.isString());
    assertEquals("test_value", result.getStringOrEmpty());
  }

  @Test
  public void testGetValueByKey_StringCollection() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key"))
        .thenReturn(Arrays.asList("value1", "value2"));

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return collection value", result.isCollection());
    assertEquals(Arrays.asList("value1", "value2"), result.getCollectionOrEmpty());
  }

  @Test
  public void testGetValueByKey_EmptyCollection() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key")).thenReturn(Collections.emptyList());

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return collection value", result.isCollection());
    assertTrue("Should return empty collection", result.getCollectionOrEmpty().isEmpty());
  }

  @Test
  public void testGetValueByKey_MixedTypeCollection() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key")).thenReturn(Arrays.asList("string", 123));

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return collection value", result.isCollection());
    assertTrue(
        "Should return empty collection for mixed types", result.getCollectionOrEmpty().isEmpty());
  }

  @Test
  public void testGetValueByKey_UnsupportedType() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key")).thenReturn(123);

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return empty string value for unsupported type", result.isString());
    assertEquals("", result.getStringOrEmpty());
  }

  @Test
  public void testGetValueByKey_ThrowsException() {
    // Arrange
    when(trailService.objectValueForTrailKey("test_key"))
        .thenThrow(new RuntimeException("Test exception"));

    // Act
    AnimaXSettingValue result = settingService.getValueByKey("test_key");

    // Assert
    assertTrue("Should return empty string value when exception occurs", result.isString());
    assertEquals("", result.getStringOrEmpty());
  }
}
