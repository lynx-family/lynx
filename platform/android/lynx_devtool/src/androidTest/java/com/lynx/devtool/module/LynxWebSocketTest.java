// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 MIT License

 Copyright (c) Meta Platforms, Inc. and affiliates.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

package com.lynx.devtool.module;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import android.util.Pair;
import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.WritableMap;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Semaphore;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okhttp3.mockwebserver.Dispatcher;
import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import okhttp3.mockwebserver.RecordedRequest;
import okio.ByteString;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxWebSocketTest {
  private MockWebServer mMockedWebServer;

  @Before
  public void setUp() throws Exception {
    mMockedWebServer = new MockWebServer();
    mMockedWebServer.setDispatcher(new Dispatcher() {
      @Override
      public MockResponse dispatch(RecordedRequest request) throws InterruptedException {
        if (request.getPath().equals("/ws")) {
          // A mock implementation of an echo websocket server
          return new MockResponse().withWebSocketUpgrade(new WebSocketListener() {
            @Override
            public void onMessage(@NonNull WebSocket webSocket, @NonNull String text) {
              if (text.equals("close")) {
                webSocket.close(1000, "client request close");
                return;
              }
              // echo the message back.
              webSocket.send(text);
            }

            @Override
            public void onMessage(@NonNull WebSocket webSocket, @NonNull ByteString bytes) {
              webSocket.send(bytes);
            }
          });
        }

        return new MockResponse().setResponseCode(404);
      }
    });
  }

  @After
  public void tearDown() throws Exception {
    if (mMockedWebServer != null) {
      mMockedWebServer.close();
    }
  }

  @Test
  public void send() throws InterruptedException {
    List<Pair<String, WritableMap>> events = new ArrayList<>();
    final Semaphore semaphore = new Semaphore(0);
    LynxWebSocket socket = new LynxWebSocket((name, data) -> {
      events.add(new Pair<>(name, data));
      semaphore.release();
    });
    int socketID = 0;
    socket.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);

    // wait until connection opened
    semaphore.acquire();
    assertEquals(1, events.size());
    assertEquals("websocketOpen", events.get(0).first);

    socket.send("foo", socketID);
    // wait until response
    semaphore.acquire();
    assertEquals(2, events.size());
    assertEquals("websocketMessage", events.get(1).first);
    WritableMap messageEvent = events.get(1).second;
    assertNotEquals(null, messageEvent);
    assertEquals("foo", messageEvent.getString("data"));
    assertEquals("text", messageEvent.getString("type"));
  }

  @Test
  public void closeBeforeOpen() {
    Map<String, WritableMap> events = new HashMap<>();
    int socketID = 1;
    LynxWebSocket socket = new LynxWebSocket(events::put);
    socket.close(1, "mock", socketID);
    assertEquals(0, events.size());
  }

  @Test
  public void sendBeforeConnect() {
    List<Pair<String, WritableMap>> events = new ArrayList<>();
    int socketID = 2;
    LynxWebSocket socket =
        new LynxWebSocket((name, data) -> { events.add(new Pair<>(name, data)); });
    socket.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);
    socket.send("mock", socketID);

    assertEquals(2, events.size());
    assertEquals("websocketFailed", events.get(0).first);
    assertNotEquals(0, events.get(0).second.getString("message").length());

    assertEquals("websocketClosed", events.get(1).first);
    assertEquals(0, events.get(1).second.getDouble("code"), 0.01);
    assertNotEquals(0, events.get(1).second.getString("reason").length());
  }

  @Test
  public void ping() throws InterruptedException {
    List<Pair<String, WritableMap>> events = new ArrayList<>();
    final Semaphore semaphore = new Semaphore(0);
    LynxWebSocket socket = new LynxWebSocket((name, data) -> {
      events.add(new Pair<>(name, data));
      semaphore.release();
    });
    int socketID = 3;
    socket.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);

    // wait until connection opened
    semaphore.acquire();
    assertEquals(1, events.size());
    assertEquals("websocketOpen", events.get(0).first);

    socket.ping(socketID);
    // wait until response
    semaphore.acquire();

    assertEquals(2, events.size());
    assertEquals("websocketMessage", events.get(1).first);
    assertEquals("binary", events.get(1).second.getString("type"));
    assertEquals("", events.get(1).second.getString("data"));
  }

  @Test
  public void clientRequestClose() throws InterruptedException {
    List<Pair<String, WritableMap>> events = new ArrayList<>();
    final Semaphore semaphore = new Semaphore(0);
    LynxWebSocket socket = new LynxWebSocket((name, data) -> {
      events.add(new Pair<>(name, data));
      semaphore.release();
    });
    int socketID = 4;
    socket.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);

    // wait until connection opened
    semaphore.acquire();
    assertEquals(1, events.size());
    assertEquals("websocketOpen", events.get(0).first);

    socket.send("close", socketID);
    semaphore.acquire();
    assertEquals(2, events.size());
    assertEquals("websocketClosed", events.get(1).first);
    assertEquals("client request close", events.get(1).second.getString("reason"));
    assertEquals(1000, events.get(1).second.getDouble("code"), 0.01);
  }

  @Test
  public void serverClose() throws InterruptedException, IOException {
    List<Pair<String, WritableMap>> events = new ArrayList<>();
    final Semaphore semaphore = new Semaphore(0);
    LynxWebSocket socket = new LynxWebSocket((name, data) -> {
      events.add(new Pair<>(name, data));
      semaphore.release();
    });
    int socketID = 3;
    socket.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);

    // wait until connection opened
    semaphore.acquire();
    assertEquals(1, events.size());
    assertEquals("websocketOpen", events.get(0).first);

    mMockedWebServer.close();
    mMockedWebServer = null;

    semaphore.acquire();
    assertEquals(2, events.size());
    assertEquals("websocketFailed", events.get(1).first);
  }
}
