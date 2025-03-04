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

import androidx.annotation.Nullable;
import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.behavior.LynxContext;

public class LynxWebSocketModule extends LynxContextModule implements LynxWebSocket.Delegate {
  public static final String NAME = "LynxWebSocketModule";

  private final LynxWebSocket mWebSocket;

  public LynxWebSocketModule(LynxContext context) {
    super(context);
    mWebSocket = new LynxWebSocket(this);
  }

  @Override
  public void destroy() {
    super.destroy();
    mWebSocket.destroy();
  }

  @LynxMethod
  public void connect(final String url, @Nullable final ReadableArray protocols,
      @Nullable final ReadableMap options, final double socketID) {
    mWebSocket.connect(url, protocols, options, socketID);
  }

  @LynxMethod
  public void close(double code, String reason, double socketID) {
    mWebSocket.close(code, reason, socketID);
  }

  @LynxMethod
  public void send(String message, double socketID) {
    mWebSocket.send(message, socketID);
  }

  @LynxMethod
  public void ping(double socketID) {
    mWebSocket.ping(socketID);
  }

  @Override
  public void sendEvent(String name, WritableMap data) {
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushMap(data);
    mLynxContext.sendGlobalEvent(name, array);
  }
}
