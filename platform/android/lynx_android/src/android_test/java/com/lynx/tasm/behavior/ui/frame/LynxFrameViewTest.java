// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.DynamicFromMap;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.UIBody;
import java.lang.reflect.Field;
import java.util.Map;
import junit.framework.TestCase;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxFrameViewTest extends TestCase {
  private LynxContext createLynxContext(
      DisplayMetrics screenMetrics, EventEmitter eventEmitter, LynxUIOwner uiOwner) {
    Context androidContext = ApplicationProvider.getApplicationContext();
    LynxContext lynxContext = new LynxContext(androidContext, screenMetrics) {
      @Override
      public void handleException(Exception e) {}

      @Override
      public void findShadowNodeAndRunTask(int sign, LynxConsumer<ShadowNode> task) {}
    };
    UIBody uiBody = mock(UIBody.class);
    when(uiBody.getBodyView()).thenReturn(null);
    lynxContext.setUIBody(uiBody);
    lynxContext.setEventEmitter(eventEmitter);
    lynxContext.setLynxUIOwner(uiOwner);
    return lynxContext;
  }

  private UIFrame createFrameUI(EventEmitter eventEmitter, LynxUIOwner uiOwner) {
    DisplayMetrics screenMetrics = new DisplayMetrics();
    screenMetrics.density = 2.0f;
    LynxContext lynxContext = createLynxContext(screenMetrics, eventEmitter, uiOwner);
    UIFrame uiFrame = new UIFrame(lynxContext);
    uiFrame.setSign(100, "frame");
    uiFrame.getView().setUrl("lynx://frame");
    when(uiOwner.findLynxUIBySign(100)).thenReturn(uiFrame);
    return uiFrame;
  }

  private TemplateData getPendingInitData(LynxFrameView frameView) throws Exception {
    Field field = LynxFrameView.class.getDeclaredField("mInitData");
    field.setAccessible(true);
    return (TemplateData) field.get(frameView);
  }

  @Test
  public void setDataMapStoresPendingTemplateData() throws Exception {
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);

    JavaOnlyMap data = new JavaOnlyMap();
    data.putInt("value", 2);
    JavaOnlyMap props = new JavaOnlyMap();
    props.putMap("data", data);

    uiFrame.setData(new DynamicFromMap(props, "data"));

    TemplateData pendingData = getPendingInitData(uiFrame.getView());
    assertNotNull(pendingData);
    Map<Object, Object> pendingMap = pendingData.toMap();
    assertEquals(2.0, ((Number) pendingMap.get("value")).doubleValue(), 0.0);
    pendingData.recycle();
  }
}
