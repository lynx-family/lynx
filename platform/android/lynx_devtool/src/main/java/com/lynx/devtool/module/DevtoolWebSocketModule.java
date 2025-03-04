// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import com.lynx.devtoolwrapper.CustomizedMessage;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwnerNG;
import com.lynx.devtoolwrapper.MessageHandler;
import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.behavior.LynxContext;
import java.lang.ref.WeakReference;

class DevToolWebSocketMessageHandler implements MessageHandler {
  WeakReference<LynxContext> mLynxContext;
  String mEventName;

  public DevToolWebSocketMessageHandler(String eventName, LynxContext context) {
    mLynxContext = new WeakReference(context);
    mEventName = eventName;
  }

  public void onMessage(String message) {
    JavaOnlyArray args = new JavaOnlyArray();
    args.pushString(message);
    LynxContext lynxContext = mLynxContext.get();
    if (lynxContext != null) {
      lynxContext.sendGlobalEvent(mEventName, args);
    }
  }
}

public class DevtoolWebSocketModule extends LynxContextModule {
  public static final String NAME = "DevtoolWebSocketModule";

  private DevToolWebSocketMessageHandler mRemoteCallHandler = null;
  private DevToolWebSocketMessageHandler mReactDevToolHandler = null;

  public DevtoolWebSocketModule(LynxContext context) {
    super(context);
    LynxBaseInspectorOwner owner = this.mLynxContext.getBaseInspectorOwner();
    if (owner instanceof LynxBaseInspectorOwnerNG) {
      mRemoteCallHandler = new DevToolWebSocketMessageHandler("OnRemoteCallMessage", context);
      ((LynxBaseInspectorOwnerNG) owner).subscribeMessage("RemoteCall", mRemoteCallHandler);
      mReactDevToolHandler = new DevToolWebSocketMessageHandler("OnReactDevtoolMessage", context);
      ((LynxBaseInspectorOwnerNG) owner).subscribeMessage("ReactDevtool", mReactDevToolHandler);
    }
  }

  @LynxMethod
  public void postMessage(final String message, final String type, final int mark) {
    LynxBaseInspectorOwner owner = this.mLynxContext.getBaseInspectorOwner();
    if (owner instanceof LynxBaseInspectorOwnerNG) {
      CustomizedMessage msg = new CustomizedMessage(type, message, mark);
      ((LynxBaseInspectorOwnerNG) owner).sendMessage(msg);
    }
  }

  @LynxMethod
  public void postMessage(final String message, final String type) {
    this.postMessage(message, type, -1);
  }
}
