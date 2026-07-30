// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;

import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxEventEmitter;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.scroll.UIScrollView;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PanInterceptAncestorsTest {
  private LynxContext mContext;

  private interface PanInterceptAncestorsSetter {
    void set(boolean value);
  }

  private static class RecordingParent extends FrameLayout {
    private final ArrayList<Boolean> mDisallowInterceptRequests = new ArrayList<>();

    RecordingParent(LynxContext context) {
      super(context);
    }

    @Override
    public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
      mDisallowInterceptRequests.add(disallowIntercept);
    }
  }

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    LynxTemplateRender renderer = mock(LynxTemplateRender.class);
    mContext.setEventEmitter(new LynxEventEmitter(renderer.getEngineProxy()));
  }

  @Test
  public void scrollViewDisallowsAndRestoresAncestorIntercept() {
    AndroidScrollView scrollView = new AndroidScrollView(mContext, mock(UIScrollView.class));

    assertDisallowsAndRestoresAncestorIntercept(scrollView, scrollView::setPanInterceptAncestors);
  }

  @Test
  public void listDisallowsAndRestoresAncestorIntercept() {
    UIList list = new UIList(mContext);

    assertDisallowsAndRestoresAncestorIntercept(list.getView(), list::setPanInterceptAncestors);
  }

  @Test
  public void listContainerDisallowsAndRestoresAncestorIntercept() {
    UIListContainer listContainer = new UIListContainer(mContext);

    assertDisallowsAndRestoresAncestorIntercept(
        listContainer.getView(), listContainer::setPanInterceptAncestors);
  }

  private void assertDisallowsAndRestoresAncestorIntercept(
      View view, PanInterceptAncestorsSetter setter) {
    RecordingParent parent = new RecordingParent(mContext);
    parent.addView(view);

    setter.set(true);
    dispatchDownEvent(view);
    assertEquals(Boolean.TRUE, parent.mDisallowInterceptRequests.get(0));

    int requestCount = parent.mDisallowInterceptRequests.size();
    setter.set(false);
    dispatchDownEvent(view);
    assertEquals(Boolean.FALSE, parent.mDisallowInterceptRequests.get(requestCount));
  }

  private void dispatchDownEvent(View view) {
    long eventTime = SystemClock.uptimeMillis();
    MotionEvent event = MotionEvent.obtain(eventTime, eventTime, MotionEvent.ACTION_DOWN, 0, 0, 0);
    view.dispatchTouchEvent(event);
    event.recycle();
  }
}
