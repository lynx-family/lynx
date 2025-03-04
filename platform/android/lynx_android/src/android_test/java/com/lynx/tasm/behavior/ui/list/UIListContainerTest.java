// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEventEmitter;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UIListContainerTest {
  private LynxContext lynxContext;
  @Before
  public void setUp() throws Exception {
    lynxContext = TestingUtils.getLynxContext();
    LynxTemplateRender renderer = mock(LynxTemplateRender.class);
    lynxContext.setEventEmitter(new LynxEventEmitter(renderer.getEngineProxy()));
    UIBody mUIBody = new UIBody(lynxContext, new UIBody.UIBodyView(lynxContext));
    LynxView mLynxView = new LynxView(lynxContext, new LynxViewBuilder());
    lynxContext.setLynxView(mLynxView);
  }
  @Test
  public void testSnap() {
    UIListContainer uiList = new UIListContainer(lynxContext);
    UIListContainer spyUIList = spy(uiList);
    JavaOnlyMap map = new JavaOnlyMap();
    map.put("offset", 20);
    map.put("factor", 0);
    uiList.setPagingAlignment((ReadableMap) map);
    map.put("offset", 20);
    map.put("factor", -1);
    uiList.setPagingAlignment((ReadableMap) map);
    uiList.willSnapTo(0, 0, 0, 0, 10);
  }
}
