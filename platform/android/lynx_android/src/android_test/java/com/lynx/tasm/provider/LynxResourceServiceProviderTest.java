// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.ILynxResourceServiceRequestOperation;
import com.lynx.tasm.service.ILynxResourceServiceResponse;
import com.lynx.tasm.service.LynxResourceServiceCallback;
import com.lynx.tasm.service.LynxResourceServiceRequestParams;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

@Ignore("Needs to be removed.")
@RunWith(AndroidJUnit4.class)
public class LynxResourceServiceProviderTest {
  private LynxResourceServiceProvider mLynxResourceServiceProvider =
      new LynxResourceServiceProvider();
  private static ILynxResourceService sLynxResourceServiceProxyMock =
      mock(ILynxResourceService.class);
  private ILynxResourceServiceRequestOperation mLynxResourceServiceRequestOperationMock =
      mock(ILynxResourceServiceRequestOperation.class);
  private byte[] mData = new byte[1];
  private ILynxResourceServiceResponse mServiceResponseData =
      mock(ILynxResourceServiceResponse.class);
  private final String mUrlRight = "right";

  @Before
  public void setUp() {
    clearInitialization();

    // Register LynxResourceService with the mock LynxResourceServiceProxy
    LynxServiceCenter.inst().registerService(
        ILynxResourceService.class, sLynxResourceServiceProxyMock);
  }

  @After
  public void tearDown() {
    clearInitialization();
  }

  private void clearInitialization() {
    try {
      // Clear LynxResourceServiceProvider initialization properties.
      Field initializedField = LynxResourceServiceProvider.class.getDeclaredField("sInitialized");
      Field serviceProxyField = LynxResourceServiceProvider.class.getDeclaredField("sServiceProxy");
      initializedField.setAccessible(true);
      serviceProxyField.setAccessible(true);
      initializedField.set(null, false);
      serviceProxyField.set(null, null);
    } catch (NoSuchFieldException e) {
      fail();
    } catch (IllegalAccessException e) {
      fail();
    }
  }

  @Test
  public void testRequest() {
    // Use mock data to test the success of the request
    doAnswer(new Answer() {
      @Override
      public ILynxResourceServiceRequestOperation answer(InvocationOnMock invocation)
          throws Throwable {
        LynxResourceServiceCallback serviceCallback = invocation.getArgument(2);
        serviceCallback.onResponse(mServiceResponseData);
        return mLynxResourceServiceRequestOperationMock;
      }
    })
        .when(sLynxResourceServiceProxyMock)
        .fetchResourceAsync(eq(mUrlRight), any(LynxResourceServiceRequestParams.class),
            any(LynxResourceServiceCallback.class));

    LynxResourceCallback<ILynxResourceResponseDataInfo> callbackSucceed =
        new LynxResourceCallback<ILynxResourceResponseDataInfo>() {
          @Override
          public void onResponse(
              @NonNull LynxResourceResponse<ILynxResourceResponseDataInfo> response) {
            super.onResponse(response);
            assertThat(response.getData().isSucceed(), equalTo(true));
            assertThat(response.getData().provideBytes(), is(equalTo(mData)));
          }
        };
    when(mServiceResponseData.isSucceed()).thenReturn(true);
    when(mServiceResponseData.provideBytes()).thenReturn(mData);
    mLynxResourceServiceProvider.request(
        new LynxResourceRequest(
            mUrlRight, LynxResourceRequest.LynxResourceType.LynxResourceTypeFont),
        callbackSucceed);

    // Use mock data to test the failure of the request
    LynxResourceCallback<ILynxResourceResponseDataInfo> callbackRequestFailed =
        new LynxResourceCallback<ILynxResourceResponseDataInfo>() {
          @Override
          public void onResponse(
              @NonNull LynxResourceResponse<ILynxResourceResponseDataInfo> response) {
            super.onResponse(response);
            assertThat(response.getCode(),
                equalTo(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED));
          }
        };
    when(mServiceResponseData.isSucceed()).thenReturn(false);
    when(mServiceResponseData.getErrorCode())
        .thenReturn(ILynxResourceService.RESULT_IS_NOT_LOCAL_RESOURCE);
    when(mServiceResponseData.getErrorInfoString()).thenReturn("fetch failed");
    mLynxResourceServiceProvider.request(
        new LynxResourceRequest(
            mUrlRight, LynxResourceRequest.LynxResourceType.LynxResourceTypeFont),
        callbackRequestFailed);

    LynxResourceCallback<ILynxResourceResponseDataInfo> callbackLynxServiceException =
        new LynxResourceCallback<ILynxResourceResponseDataInfo>() {
          @Override
          public void onResponse(
              @NonNull LynxResourceResponse<ILynxResourceResponseDataInfo> response) {
            super.onResponse(response);
            assertThat(response.getCode(),
                equalTo(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED));
          }
        };
    when(mServiceResponseData.isSucceed()).thenReturn(false);
    when(mServiceResponseData.getErrorCode()).thenReturn(ILynxResourceService.RESULT_EXCEPTION);
    when(mServiceResponseData.getErrorInfoString()).thenReturn("result exception");
    mLynxResourceServiceProvider.request(
        new LynxResourceRequest(
            mUrlRight, LynxResourceRequest.LynxResourceType.LynxResourceTypeFont),
        callbackLynxServiceException);
  }

  @Test
  public void testRequestSync() {
    when(sLynxResourceServiceProxyMock.fetchResourceSync(
             eq(mUrlRight), any(LynxResourceServiceRequestParams.class)))
        .thenReturn(mServiceResponseData);

    // Use mock data to test the success of the request
    when(mServiceResponseData.isSucceed()).thenReturn(true);
    when(mServiceResponseData.provideBytes()).thenReturn(mData);
    LynxResourceResponse<ILynxResourceResponseDataInfo> response =
        mLynxResourceServiceProvider.requestSync(new LynxResourceRequest(
            mUrlRight, LynxResourceRequest.LynxResourceType.LynxResourceTypeFont));
    assertThat(response.getData().isSucceed(), equalTo(true));
    assertThat(response.getData().provideBytes(), is(equalTo(mData)));

    // Use mock data to test the failure of the request
    when(mServiceResponseData.isSucceed()).thenReturn(false);
    when(mServiceResponseData.getErrorCode())
        .thenReturn(ILynxResourceService.RESULT_IS_NOT_LOCAL_RESOURCE);
    when(mServiceResponseData.getErrorInfoString()).thenReturn("fetch failed");
    response = mLynxResourceServiceProvider.requestSync(new LynxResourceRequest(
        mUrlRight, LynxResourceRequest.LynxResourceType.LynxResourceTypeFont));
    assertThat(
        response.getCode(), equalTo(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED));

    when(mServiceResponseData.isSucceed()).thenReturn(false);
    when(mServiceResponseData.getErrorCode()).thenReturn(ILynxResourceService.RESULT_EXCEPTION);
    when(mServiceResponseData.getErrorInfoString()).thenReturn("result exception");
    response = mLynxResourceServiceProvider.requestSync(new LynxResourceRequest(
        mUrlRight, LynxResourceRequest.LynxResourceType.LynxResourceTypeFont));
    assertThat(
        response.getCode(), equalTo(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED));
  }
}
