// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.jsbridge.Arguments;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okhttp3.mockwebserver.Dispatcher;
import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import okhttp3.mockwebserver.RecordedRequest;
import okio.ByteString;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxWebSocketModuleTest {
  private LynxContext mContext;
  private MockWebServer mMockedWebServer;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();

    mMockedWebServer = new MockWebServer();
    mMockedWebServer.setDispatcher(new Dispatcher() {
      @Override
      public MockResponse dispatch(RecordedRequest request) {
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

  @Test
  public void destroy() {
    int socketID = 0;
    LynxWebSocketModule module = new LynxWebSocketModule(mContext);
    module.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);
    module.destroy();
  }

  @Test
  public void connect() {
    LynxWebSocketModule module = new LynxWebSocketModule(mContext);
    module.connect(mMockedWebServer.url("/ws").toString(), null, null, 0);
  }

  @Test
  public void close() {
    int socketID = 0;
    LynxWebSocketModule module = new LynxWebSocketModule(mContext);
    module.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);
    module.close(1000, "test", socketID);
  }

  @Test
  public void send() {
    int socketID = 1;
    LynxWebSocketModule module = new LynxWebSocketModule(mContext);
    module.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);
    module.send("test message", socketID);
  }

  @Test
  public void ping() {
    int socketID = 2;
    LynxWebSocketModule module = new LynxWebSocketModule(mContext);
    module.connect(mMockedWebServer.url("/ws").toString(), null, null, socketID);
    module.ping(socketID);
  }

  @Test
  public void sendEvent() {
    int socketID = 3;
    LynxWebSocketModule module = new LynxWebSocketModule(mContext);

    WritableMap params = Arguments.createMap();
    params.putInt("id", socketID);
    module.sendEvent("message", params);
  }
}
