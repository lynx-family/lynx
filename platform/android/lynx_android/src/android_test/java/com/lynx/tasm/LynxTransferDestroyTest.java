// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.app.Application;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.behavior.ui.transfer.UITransfer;
import com.lynx.tasm.transfer.LynxTransferListener;
import java.lang.reflect.Field;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxTransferDestroyTest {
  @Test
  @UiThreadTest
  public void destroyRemovesTransferAfterTemplateContextIsCleared() throws Exception {
    Application application = (Application) InstrumentationRegistry.getInstrumentation()
                                  .getTargetContext()
                                  .getApplicationContext();
    LynxEnv.inst().init(application, null, null, null, null);
    LynxView owner = new LynxViewBuilder().build(application);
    FrameLayout host = new FrameLayout(application);
    int[] removals = {0};
    owner.registerTransferListener(new LynxTransferListener() {
      @Override
      public boolean onCreate(String id, View view, ReadableMap dataset) {
        host.addView(view);
        return true;
      }

      @Override
      public void onDatasetUpdate(String id, ReadableMap dataset) {}

      @Override
      public void onRemove(String id, View view) {
        removals[0]++;
      }
    });
    UITransfer transfer = new UITransfer(owner.getLynxContext());
    transfer.setSign(100, "transfer-view");
    transfer.setTransferId("destroy-test");
    transfer.onNodeReady();
    View content = host.getChildAt(0);
    assertNotNull(content);

    LynxTemplateRender render = owner.mLynxTemplateRender;
    Field rendererField = LynxTemplateRender.class.getDeclaredField("mLynxUIRender");
    rendererField.setAccessible(true);
    ILynxUIRenderer original = (ILynxUIRenderer) rendererField.get(render);
    ILynxUIRenderer renderer = mock(ILynxUIRenderer.class);
    doAnswer(invocation -> {
      assertNull(owner.getLynxContext());
      transfer.destroy();
      return null;
    })
        .when(renderer)
        .onDestroy();
    rendererField.set(render, renderer);
    try {
      owner.destroy();
      verify(renderer).onDestroy();
      assertNull(content.getParent());
      assertEquals(1, removals[0]);
      assertNull(owner.getLynxContext());

      View lateContent = new View(application);
      host.addView(lateContent);
      owner.dispatchTransferRemove("destroy-test", lateContent);
      assertNull(lateContent.getParent());
    } finally {
      rendererField.set(render, original);
      original.onDestroy();
    }
  }
}
