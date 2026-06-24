// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.animax.loader;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.animax.ability.BaseAbility;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LynxHttpAnimaXLoaderTest {
  private LynxHttpAnimaXLoader loader;

  @Mock private BaseAbility mockAbility;
  @Mock private LynxContext mockContext;
  @Mock private LynxGenericResourceFetcher mockFetcher;
  @Mock private IAnimaXLoaderCompletionHandler mockCompletionHandler;
  @Mock private IAnimaXLoaderRequest mockRequest;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    when(mockContext.getGenericResourceFetcher()).thenReturn(mockFetcher);
    loader = new LynxHttpAnimaXLoader(mockAbility, mockContext);
  }

  @Test
  public void testGetScheme() {
    assertEquals(AnimaXLoaderScheme.HTTP, loader.getScheme());
  }

  @Test
  public void testLoadWithGenericFetcher_Success() {
    // Arrange
    String testUrl = "https://example.com/test.json";
    byte[] testData = "test data".getBytes();
    when(mockRequest.getUri()).thenReturn(testUrl);
    when(mockRequest.getImageInfo()).thenReturn(null);

    // Act
    loader.load(mockRequest, mockCompletionHandler);

    // Capture the callback
    ArgumentCaptor<LynxResourceCallback<byte[]>> callbackCaptor =
        ArgumentCaptor.forClass(LynxResourceCallback.class);
    verify(mockFetcher).fetchResource(any(), callbackCaptor.capture());

    // Create actual response instead of mocking
    LynxResourceResponse<byte[]> response = LynxResourceResponse.onSuccess(testData);
    callbackCaptor.getValue().onResponse(response);

    // Assert
    ArgumentCaptor<AnimaXLoaderResponse<?>> responseCaptor =
        ArgumentCaptor.forClass(AnimaXLoaderResponse.class);
    verify(mockCompletionHandler).onComplete(responseCaptor.capture());

    AnimaXLoaderResponse<?> capturedResponse = responseCaptor.getValue();
    assertArrayEquals(testData, (byte[]) capturedResponse.getData());
  }

  @Test
  public void testLoadWithGenericFetcher_Error() {
    // Arrange
    String testUrl = "https://example.com/test.json";
    when(mockRequest.getUri()).thenReturn(testUrl);
    when(mockRequest.getImageInfo()).thenReturn(null);

    // Act
    loader.load(mockRequest, mockCompletionHandler);

    // Capture the callback
    ArgumentCaptor<LynxResourceCallback<byte[]>> callbackCaptor =
        ArgumentCaptor.forClass(LynxResourceCallback.class);
    verify(mockFetcher).fetchResource(any(), callbackCaptor.capture());

    // Create actual error response instead of mocking
    Exception testError = new Exception("Test error");
    LynxResourceResponse response = LynxResourceResponse.onFailed(testError);
    callbackCaptor.getValue().onResponse(response);

    // Assert
    ArgumentCaptor<AnimaXLoaderResponse<?>> responseCaptor =
        ArgumentCaptor.forClass(AnimaXLoaderResponse.class);
    verify(mockCompletionHandler).onComplete(responseCaptor.capture());

    AnimaXLoaderResponse<?> capturedResponse = responseCaptor.getValue();
    assertSame(capturedResponse.getType(), AnimaXLoaderResponse.Type.ERROR);
    assertNotNull(capturedResponse.getData());
  }

  @Test
  public void testLoadImageRequest() {
    // Arrange
    String testUrl = "https://example.com/test.jpg";
    when(mockRequest.getUri()).thenReturn(testUrl);
    when(mockRequest.getImageInfo()).thenReturn(mock(IAnimaXLoaderRequest.IImageInfo.class));
    when(mockAbility.redirectUrl(testUrl)).thenReturn("https://example.com/redirected.jpg");

    // Act
    loader.load(mockRequest, mockCompletionHandler);

    // Assert
    verify(mockFetcher, never()).fetchResource(any(), any());
    // Note: Further assertions would depend on image service implementation
  }

  @Test
  public void testLoadWithNullFetcher() {
    // Arrange
    when(mockContext.getGenericResourceFetcher()).thenReturn(null);
    when(mockRequest.getUri()).thenReturn("https://example.com/test.json");
    when(mockRequest.getImageInfo()).thenReturn(null);

    // Act
    loader.load(mockRequest, mockCompletionHandler);

    // Assert
    verify(mockFetcher, never()).fetchResource(any(), any());
  }
}
