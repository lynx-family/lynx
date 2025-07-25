// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsptracing;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

import junit.framework.TestCase;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.test.platform.app.InstrumentationRegistry;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.LynxUIRenderer;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.ResourceHelper;

import java.util.ArrayList;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class LynxFSPTracerTest extends TestCase {
  private LynxView mLynxView;
  private LynxContext mContext;
  private FSPTracer mTracer;
  private FSPTracer.IObserverCallback mObserverCallback;
  private FSPTracer.ICompletionCallback mCompletionCallback;
  private final List<String> callSequence = new ArrayList<>();

  private static class FSPCallback implements FSPTracer.IObserverCallback {
    private final List<String> callSequence;

    FSPCallback(List<String> callSequence) {
      this.callSequence = callSequence;
    }

    @Override
    public void onStart() {
      callSequence.add("onStart");
    }

    @Override
    public void onBecomeStable(boolean stable, @NonNull FSPResult result) {
      callSequence.add("onBecomeStable");
    }

    @Override
    public void onStop(@NonNull FSPTracer.StopReason reason, @NonNull FSPResult result) {
      callSequence.add("onStop");
    }

    @Override
    public void willStartGracefulTimeout(double timeout) {
      callSequence.add("willStartGracefulTimeout");
    }
  }

  private static class LynxFSPCompletionCallback implements FSPTracer.ICompletionCallback {
    private final List<String> callSequence;

    LynxFSPCompletionCallback(List<String> callSequence) {
      this.callSequence = callSequence;
    }

    @Override
    public void onComplete(@NonNull FSPResult result) {
      callSequence.add("onComplete");
    }
  }

  public LynxFSPTracerTest() {}

  @Before
  public void setUp() throws Exception {
    super.setUp();

    final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxEnv.inst().init((Application) context, System::loadLibrary, null, null, null);

    Map<String, String> schemaMap = new HashMap<>();
    schemaMap.put("test", "true");

    mContext = TestingUtils.getLynxContext();
    mLynxView = new LynxViewBuilder().setLynxViewConfig(schemaMap).build(mContext);

    LynxUIOwner uiOwner = (LynxUIOwner)mLynxView.lynxUIRenderer().lynxUIOwner();
    try {
      Method getFSPTracer = uiOwner.getClass().getDeclaredMethod("getFSPTracer");
      getFSPTracer.setAccessible(true);
      if (getFSPTracer.invoke(uiOwner) instanceof FSPTracer) {
        mTracer = (FSPTracer)getFSPTracer.invoke(uiOwner);
      } else {
        mTracer = null;
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    
    mObserverCallback = new FSPCallback(callSequence);
    mCompletionCallback = new LynxFSPCompletionCallback(callSequence);
    
    mTracer.setObserverCallback(mObserverCallback);
  }

  @Test
  public void testFSPTracer() throws Exception {
    byte[] templates = ResourceHelper.loadByteArray(mContext, "hello-world.js");
    mLynxView.renderTemplate(templates, new HashMap<>());

    System.out.println(callSequence);
  }
}
