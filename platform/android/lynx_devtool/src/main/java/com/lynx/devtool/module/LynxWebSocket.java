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

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.jsbridge.Arguments;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.base.LLog;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okio.ByteString;

public class LynxWebSocket {
  public interface Delegate {
    void sendEvent(String name, WritableMap data);
  }

  private final Delegate mDelegate;

  static final String TAG = "LynxWebSocket";

  public interface ContentHandler {
    void onMessage(String text, WritableMap params);

    void onMessage(ByteString byteString, WritableMap params);
  }

  private final Map<Integer, WebSocket> mWebSocketConnections = new ConcurrentHashMap<>();
  private final Map<Integer, LynxWebSocket.ContentHandler> mContentHandlers =
      new ConcurrentHashMap<>();

  public void setContentHandler(final int id, final LynxWebSocket.ContentHandler contentHandler) {
    if (contentHandler != null) {
      mContentHandlers.put(id, contentHandler);
    } else {
      mContentHandlers.remove(id);
    }
  }

  public LynxWebSocket(Delegate delegate) {
    mDelegate = delegate;
  }

  public void destroy() {
    for (Map.Entry<Integer, WebSocket> entry : mWebSocketConnections.entrySet()) {
      entry.getValue().close(1000, "Destroy");
    }
  }

  public void connect(final String url, @Nullable final ReadableArray protocols,
      @Nullable final ReadableMap ignoredOptions, final double socketID) {
    final int id = (int) socketID;
    OkHttpClient.Builder okHttpBuilder =
        new OkHttpClient.Builder()
            .connectTimeout(10, TimeUnit.SECONDS)
            .writeTimeout(10, TimeUnit.SECONDS)
            .readTimeout(0, TimeUnit.MINUTES); // Disable timeouts for read

    OkHttpClient client = okHttpBuilder.build();

    Request.Builder builder = new Request.Builder().tag(id).url(url);

    boolean hasOriginHeader = false;

    // TODO: support options.headers

    if (protocols != null && protocols.size() > 0) {
      StringBuilder protocolsValue = new StringBuilder();
      for (int i = 0; i < protocols.size(); i++) {
        String v = protocols.getString(i).trim();
        if (!v.isEmpty() && !v.contains(",")) {
          protocolsValue.append(v);
          protocolsValue.append(",");
        }
      }
      if (protocolsValue.length() > 0) {
        protocolsValue.replace(protocolsValue.length() - 1, protocolsValue.length(), "");
        builder.addHeader("Sec-WebSocket-Protocol", protocolsValue.toString());
      }
    }

    client.newWebSocket(builder.build(), new WebSocketListener() {
      @Override
      public void onOpen(@NonNull WebSocket webSocket, @NonNull Response response) {
        mWebSocketConnections.put(id, webSocket);
        WritableMap params = Arguments.createMap();
        params.putInt("id", id);
        params.putString("protocol", response.header("Sec-WebSocket-Protocol", ""));
        sendEvent("websocketOpen", params);
      }

      @Override
      public void onClosing(@NonNull WebSocket websocket, int code, @NonNull String reason) {
        websocket.close(code, reason);
      }

      @Override
      public void onClosed(@NonNull WebSocket webSocket, int code, @NonNull String reason) {
        WritableMap params = Arguments.createMap();
        params.putInt("id", id);
        params.putInt("code", code);
        params.putString("reason", reason);
        sendEvent("websocketClosed", params);
      }

      @Override
      public void onFailure(@NonNull WebSocket webSocket, @NonNull Throwable t, Response response) {
        notifyWebSocketFailed(id, t.getMessage());
      }

      @Override
      public void onMessage(@NonNull WebSocket webSocket, @NonNull String text) {
        WritableMap params = Arguments.createMap();
        params.putInt("id", id);
        params.putString("type", "text");
        LynxWebSocket.ContentHandler contentHandler = mContentHandlers.get(id);
        if (contentHandler != null) {
          contentHandler.onMessage(text, params);
        } else {
          params.putString("data", text);
        }
        sendEvent("websocketMessage", params);
      }

      @Override
      public void onMessage(@NonNull WebSocket webSocket, @NonNull ByteString bytes) {
        WritableMap params = Arguments.createMap();
        params.putInt("id", id);
        params.putString("type", "binary");
        LynxWebSocket.ContentHandler contentHandler = mContentHandlers.get(id);
        if (contentHandler != null) {
          contentHandler.onMessage(bytes, params);
        } else {
          String text = bytes.base64();
          params.putString("data", text);
        }
        sendEvent("websocketMessage", params);
      }
    });

    // Trigger shutdown of the dispatcher's executor so this process can exit cleanly
    client.dispatcher().executorService().shutdown();
  }

  public void close(double code, String reason, double socketID) {
    int id = (int) socketID;
    WebSocket client = mWebSocketConnections.get(id);
    if (client == null) {
      // WebSocket is already closed
      // Don't do anything, mirror the behaviour on web
      return;
    }
    try {
      client.close((int) code, reason);
      mWebSocketConnections.remove(id);
      mContentHandlers.remove(id);
    } catch (Exception e) {
      LLog.e(TAG, "Could not close WebSocket connection for id " + id + e);
    }
  }

  public void send(String message, double socketID) {
    final int id = (int) socketID;
    WebSocket client = mWebSocketConnections.get(id);
    if (client == null) {
      // This is a programmer error -- display development warning
      notifyWebSocketFailed(id, "client is null", true);
      mWebSocketConnections.remove(id);
      mContentHandlers.remove(id);
      return;
    }
    try {
      client.send(message);
    } catch (Exception e) {
      notifyWebSocketFailed(id, e.getMessage());
    }
  }

  public void ping(double socketID) {
    final int id = (int) socketID;
    WebSocket client = mWebSocketConnections.get(id);
    if (client == null) {
      // This is a programmer error -- display development warning
      notifyWebSocketFailed(id, "client is null", true);
      mWebSocketConnections.remove(id);
      mContentHandlers.remove(id);
      return;
    }
    try {
      client.send(ByteString.EMPTY);
    } catch (Exception e) {
      notifyWebSocketFailed(id, e.getMessage());
    }
  }

  private void sendEvent(String name, WritableMap data) {
    mDelegate.sendEvent(name, data);
  }

  private void notifyWebSocketFailed(int id, String message) {
    notifyWebSocketFailed(id, message, false);
  }

  private void notifyWebSocketFailed(int id, String message, boolean withClose) {
    WritableMap params = Arguments.createMap();
    params.putInt("id", id);
    params.putString("message", message);
    sendEvent("websocketFailed", params);

    if (withClose) {
      params = Arguments.createMap();
      params.putInt("id", id);
      params.putInt("code", 0);
      params.putString("reason", message);
      sendEvent("websocketClosed", params);
    }
  }
}
