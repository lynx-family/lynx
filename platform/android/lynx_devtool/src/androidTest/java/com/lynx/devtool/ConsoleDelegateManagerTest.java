// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.behavior.LynxUIOwner;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ConsoleDelegateManagerTest {
  String mMessage;
  String mObjectDetail;
  String mObjectStringify;
  DevToolPlatformAndroidDelegate mPlatformDelegate;

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();

    LynxEnv.inst().init((Application) context, null, null, null, null);
    LynxDevtoolEnv.inst().init(context);

    LynxInspectorConsoleDelegate delegate = new LynxInspectorConsoleDelegate() {
      @Override
      public void onConsoleMessage(String msg) {
        mMessage = msg;
      }
    };

    mPlatformDelegate =
        new DevToolPlatformAndroidDelegate(mock(LynxView.class), mock(LynxUIOwner.class));
    mPlatformDelegate.setLynxInspectorConsoleDelegate(delegate);
  }

  @Test
  public void onConsoleObject() {
    String message =
        "{\"type\":\"log\",\"args\":[{\"value\":\"test object: \",\"type\":\"string\"},{\"type\":\"object\",\"objectId\":\"530412529136\",\"className\":\"Object\",\"description\":\"Object\"}]}";
    mPlatformDelegate.onConsoleMessage(message);
    assertEquals(message, mMessage);

    String objectId = "530412529136";
    mPlatformDelegate.getConsoleObject(objectId, false, new Callback() {
      @Override
      public void invoke(Object... args) {
        mObjectDetail = args[0].toString();
      }
    });
    mPlatformDelegate.getConsoleObject(objectId, true, new Callback() {
      @Override
      public void invoke(Object... args) {
        mObjectStringify = args[0].toString();
      }
    });

    String detail =
        "[{\"name\":\"a\",\"value\":{\"description\":\"1\",\"value\":1,\"type\":\"number\"}},{\"name\":\"b\",\"value\":{\"value\":\"test\",\"type\":\"string\"}},{\"name\":\"c\",\"value\":{\"value\":true,\"type\":\"boolean\"}},{\"name\":\"__proto__\",\"value\":{\"type\":\"object\",\"objectId\":\"530412818672\",\"className\":\"Object\",\"description\":\"Object\"}}]";
    String stringify = "{\n\t\"a\": 1,\n\t\"b\": \"test\",\n\t\"c\": true\n}";
    mPlatformDelegate.onConsoleObject(detail, -1);
    assertEquals(detail, mObjectDetail);
    mPlatformDelegate.onConsoleObject(stringify, -2);
    assertEquals(stringify, mObjectStringify);
  }
}
