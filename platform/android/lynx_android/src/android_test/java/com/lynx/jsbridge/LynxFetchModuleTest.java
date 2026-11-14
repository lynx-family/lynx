// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.app.Application;
import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtoolwrapper.LynxNetworkRequestObserver;
import com.lynx.jsbridge.network.HttpRequest;
import com.lynx.jsbridge.network.HttpResponse;
import com.lynx.jsbridge.network.HttpStreamingDelegate;
import com.lynx.jsbridge.network.LynxFetchModuleEventSender;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.service.ILynxHttpRequestOperation;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.LynxHttpRequestCallback;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxFetchModuleTest {
  private static class MockNetworkRequestObserver implements LynxNetworkRequestObserver {
    final List<String> calls = new ArrayList<>();
    boolean enabled = true;
    HttpRequest capturedRequest;
    int responseStatus;

    @Override
    public boolean isEnabled() {
      return enabled;
    }

    @Override
    public String requestWillBeSent(String url, String method, JavaOnlyMap headers, byte[] body) {
      capturedRequest = new HttpRequest();
      capturedRequest.setUrl(url);
      capturedRequest.setHttpMethod(method);
      capturedRequest.setHttpHeaders(headers);
      capturedRequest.setHttpBody(body);
      calls.add("request");
      return "request-1";
    }

    @Override
    public void responseReceived(
        String requestId, String url, int status, String statusText, JavaOnlyMap headers) {
      responseStatus = status;
      calls.add("response:" + requestId);
    }

    @Override
    public void dataReceived(String requestId, byte[] data) {
      calls.add("data:" + requestId + ":" + new String(data));
    }

    @Override
    public void loadingFinished(String requestId) {
      calls.add("finished:" + requestId);
    }

    @Override
    public void loadingFailed(String requestId, String errorText, boolean canceled) {
      calls.add("failed:" + requestId + ":" + errorText);
    }
  }

  private static class MockEventSender extends LynxFetchModuleEventSender {
    final MockNetworkRequestObserver observer = new MockNetworkRequestObserver();
    final List<JavaOnlyMap> streamingEvents = new ArrayList<>();

    @Override
    public LynxNetworkRequestObserver getNetworkRequestObserver() {
      return observer;
    }

    @Override
    public void sendGlobalEvent(String name, JavaOnlyArray params) {
      streamingEvents.add(params.getMap(0));
    }
  }

  private static class MockHttpService implements ILynxHttpService {
    HttpResponse response;
    HttpRequest capturedRequest;
    byte[][] streamingData = new byte[0][];

    @Override
    public void requestStreaming(
        HttpRequest request, LynxHttpRequestCallback callback, HttpStreamingDelegate delegate) {
      capturedRequest = request;
      callback.invoke(response);
      for (byte[] data : streamingData) {
        delegate.onData(data);
      }
      delegate.onEnd();
    }

    @Override
    public void request(HttpRequest request, LynxHttpRequestCallback callback) {
      capturedRequest = request;
      callback.invoke(response);
    }
  }

  private static class MockCancelableHttpService extends MockHttpService {
    LynxHttpRequestCallback pendingCallback;
    HttpStreamingDelegate pendingDelegate;
    int cancelCount;

    @Override
    public ILynxHttpRequestOperation requestStreamingCancelable(
        HttpRequest request, LynxHttpRequestCallback callback, HttpStreamingDelegate delegate) {
      capturedRequest = request;
      pendingCallback = callback;
      pendingDelegate = delegate;
      return () -> cancelCount++;
    }

    @Override
    public ILynxHttpRequestOperation requestCancelable(
        HttpRequest request, LynxHttpRequestCallback callback) {
      capturedRequest = request;
      pendingCallback = callback;
      return () -> cancelCount++;
    }
  }

  private Context context;
  private MockEventSender sender;
  private MockHttpService httpService;
  private LynxFetchModule module;

  @Before
  public void setUp() {
    context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxServiceCenter.inst().initialize((Application) context);
    sender = new MockEventSender();
    httpService = new MockHttpService();
    LynxServiceCenter.inst().registerService(httpService);
    module = new LynxFetchModule(context, sender);
  }

  @After
  public void tearDown() {
    LynxServiceCenter.inst().unregisterService(ILynxHttpService.class);
  }

  private JavaOnlyMap createRequest(boolean streaming, String requestId) {
    JavaOnlyMap request = createRequest(streaming);
    request.putString("requestId", requestId);
    return request;
  }

  private JavaOnlyMap createRequest(boolean streaming) {
    JavaOnlyMap headers = new JavaOnlyMap();
    headers.putString("X-Test", "request-header");
    JavaOnlyMap extension = new JavaOnlyMap();
    extension.putBoolean("useStreaming", streaming);
    JavaOnlyMap request = new JavaOnlyMap();
    request.putString("method", "POST");
    request.putString("url", "https://example.com/fetch");
    request.putString("origin", "https://example.com/page");
    request.putMap("headers", headers);
    request.putByteArray("body", "request-body".getBytes());
    request.putMap("lynxExtension", extension);
    return request;
  }

  private HttpResponse createResponse(int status, byte[] body) {
    JavaOnlyMap headers = new JavaOnlyMap();
    headers.putString("Content-Type", "text/plain");
    HttpResponse response = new HttpResponse();
    response.setUrl("https://example.com/fetch");
    response.setStatusCode(status);
    response.setStatusText("response-status");
    response.setHttpHeaders(headers);
    response.setHttpBody(body);
    return response;
  }

  @Test
  public void testNormalFetchReportsOneCompleteRequest() {
    httpService.response = createResponse(501, "response-body".getBytes());
    final Object[][] resolved = new Object[1][];

    module.fetch(createRequest(false), args -> resolved[0] = args, args -> {});

    assertNotNull(resolved[0]);
    assertEquals(4, sender.observer.calls.size());
    assertEquals("request", sender.observer.calls.get(0));
    assertEquals("response:request-1", sender.observer.calls.get(1));
    assertEquals("data:request-1:response-body", sender.observer.calls.get(2));
    assertEquals("finished:request-1", sender.observer.calls.get(3));
    assertEquals(501, sender.observer.responseStatus);
    assertEquals("POST", sender.observer.capturedRequest.getHttpMethod());
    assertEquals("https://example.com/fetch", sender.observer.capturedRequest.getUrl());
    assertArrayEquals("request-body".getBytes(), sender.observer.capturedRequest.getHttpBody());
    assertEquals(
        "request-header", sender.observer.capturedRequest.getHttpHeaders().getString("X-Test"));
  }

  @Test
  public void testStreamingFetchReportsChunksAndCompletion() {
    httpService.response = createResponse(200, new byte[0]);
    httpService.streamingData = new byte[][] {"first".getBytes(), "second".getBytes()};

    module.fetch(createRequest(true), args -> {}, args -> {});

    assertEquals(5, sender.observer.calls.size());
    assertEquals("request", sender.observer.calls.get(0));
    assertEquals("response:request-1", sender.observer.calls.get(1));
    assertEquals("data:request-1:first", sender.observer.calls.get(2));
    assertEquals("data:request-1:second", sender.observer.calls.get(3));
    assertEquals("finished:request-1", sender.observer.calls.get(4));
    assertEquals(3, sender.streamingEvents.size());
    assertEquals("onData", sender.streamingEvents.get(0).getString("event"));
    assertEquals("onData", sender.streamingEvents.get(1).getString("event"));
    assertEquals("onEnd", sender.streamingEvents.get(2).getString("event"));
  }

  @Test
  public void testCancelPendingRequestCancelsOperationOnce() {
    MockCancelableHttpService cancelableService = new MockCancelableHttpService();
    LynxServiceCenter.inst().registerService(cancelableService);

    module.fetch(createRequest(false, "1"), args -> {}, args -> {});
    assertNotNull(cancelableService.pendingCallback);
    module.cancel("unknown");
    assertEquals(0, cancelableService.cancelCount);

    module.cancel("1");
    module.cancel("1");

    assertEquals(1, cancelableService.cancelCount);
  }

  @Test
  public void testCancelFinishedRequestIsNoOp() {
    MockCancelableHttpService cancelableService = new MockCancelableHttpService();
    LynxServiceCenter.inst().registerService(cancelableService);
    final Object[][] resolved = new Object[1][];

    module.fetch(createRequest(false, "1"), args -> resolved[0] = args, args -> {});
    cancelableService.pendingCallback.invoke(createResponse(200, "response-body".getBytes()));
    module.cancel("1");

    assertNotNull(resolved[0]);
    assertEquals(0, cancelableService.cancelCount);
  }

  @Test
  public void testCancelStreamingRequestUntilEnd() {
    MockCancelableHttpService cancelableService = new MockCancelableHttpService();
    LynxServiceCenter.inst().registerService(cancelableService);

    module.fetch(createRequest(true, "1"), args -> {}, args -> {});
    // A streaming request is still in flight after the response is received.
    cancelableService.pendingCallback.invoke(createResponse(200, new byte[0]));
    module.cancel("1");
    assertEquals(1, cancelableService.cancelCount);

    module.fetch(createRequest(true, "2"), args -> {}, args -> {});
    cancelableService.pendingCallback.invoke(createResponse(200, new byte[0]));
    cancelableService.pendingDelegate.onEnd();
    module.cancel("2");
    assertEquals(1, cancelableService.cancelCount);
  }

  @Test
  public void testCancelWithoutCancelableHttpServiceIsNoOp() {
    httpService.response = createResponse(200, "response-body".getBytes());
    final Object[][] resolved = new Object[1][];

    module.fetch(createRequest(false, "1"), args -> resolved[0] = args, args -> {});
    module.cancel("1");

    assertNotNull(resolved[0]);
  }

  @Test
  public void testDisabledObserverDoesNotReportNetworkEvents() {
    sender.observer.enabled = false;
    httpService.response = createResponse(200, "response-body".getBytes());
    final Object[][] resolved = new Object[1][];

    module.fetch(createRequest(false), args -> resolved[0] = args, args -> {});

    assertNotNull(resolved[0]);
    assertEquals(0, sender.observer.calls.size());
  }

  @Test
  public void testMissingHttpServiceReportsFailure() {
    LynxServiceCenter.inst().unregisterService(ILynxHttpService.class);
    final Object[][] rejected = new Object[1][];

    module.fetch(createRequest(false), args -> {}, args -> rejected[0] = args);

    assertNotNull(rejected[0]);
    assertEquals(2, sender.observer.calls.size());
    assertEquals("request", sender.observer.calls.get(0));
    assertEquals("failed:request-1:Lynx Http Service not registered", sender.observer.calls.get(1));
    assertNull(httpService.capturedRequest);
  }
}
