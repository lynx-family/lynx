// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import com.lynx.tasm.behavior.ui.UIBody;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class LynxUIRendererTest {
  @Mock private LynxContext mLynxContext;
  @Mock private LynxUIOwner mLynxUIOwner;
  @Mock private UIBody.UIBodyView mBodyView;

  private LynxUIRenderer mRenderer;

  @Before
  public void setUp() throws Exception {
    mRenderer = new LynxUIRenderer();
    setField("mLynxContext", new WeakReference<>(mLynxContext));
    setField("mLynxUIOwner", mLynxUIOwner);
    when(mLynxContext.getUIBodyView()).thenReturn(mBodyView);
    when(mLynxContext.isFragmentLayerRenderOn()).thenReturn(true);
  }

  @Test
  public void fragmentLayerRenderMeasuresOwnedUIs() {
    mRenderer.performInnerMeasure(View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(200, View.MeasureSpec.EXACTLY));

    verify(mLynxUIOwner).performMeasure();
    verify(mBodyView).innerSetMeasuredDimension(100, 200);
  }

  @Test
  public void fragmentLayerRenderLaysOutOwnedUIs() {
    mRenderer.onLayout(false, 0, 0, 100, 200);

    verify(mLynxUIOwner).performLayout();
  }

  private void setField(String name, Object value) throws Exception {
    Field field = LynxUIRenderer.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(mRenderer, value);
  }
}
