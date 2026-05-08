// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.transfer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.same;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.graphics.Canvas;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.TouchEventDispatcher;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;

@RunWith(AndroidJUnit4.class)
public class UITransferTest {
  private static final int SIGN = 10;

  private static class TestLynxContext extends LynxContext {
    private LynxUIOwner mRetainedUIOwner;
    private ShadowNode mShadowNode;

    TestLynxContext(Context context, DisplayMetrics displayMetrics) {
      super(context, displayMetrics);
    }

    @Override
    public void handleException(Exception exception) {}

    @Override
    public LynxUIOwner getLynxUIOwner() {
      return mRetainedUIOwner;
    }

    @Override
    public void findShadowNodeAndRunTask(int sign, LynxConsumer<ShadowNode> task) {
      if (mShadowNode != null) {
        task.accept(mShadowNode);
      }
    }
  }

  private TestLynxContext createLynxContext(
      LynxView lynxView, EventEmitter eventEmitter, TouchEventDispatcher touchEventDispatcher) {
    Context context = ApplicationProvider.getApplicationContext();
    DisplayMetrics displayMetrics = new DisplayMetrics();
    displayMetrics.setTo(context.getResources().getDisplayMetrics());
    DisplayMetricsHolder.updateOrInitDisplayMetrics(context);
    TestLynxContext lynxContext = new TestLynxContext(context, displayMetrics);
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    when(uiOwner.getContext()).thenReturn(lynxContext);
    lynxContext.mRetainedUIOwner = uiOwner;
    lynxContext.setLynxUIOwner(uiOwner);
    lynxContext.setLynxView(lynxView);
    lynxContext.setEventEmitter(eventEmitter);
    lynxContext.setTouchEventDispatcher(touchEventDispatcher);
    return lynxContext;
  }

  private UITransfer createTransferUI(TestLynxContext lynxContext) {
    UITransfer transferUI = new UITransfer(lynxContext);
    transferUI.setSign(SIGN, "transfer-view");
    return transferUI;
  }

  @Test
  public void transferLifecycleCreatesDeduplicatesAndRemovesWrapper() {
    LynxView lynxView = mock(LynxView.class);
    TestLynxContext context =
        createLynxContext(lynxView, mock(EventEmitter.class), mock(TouchEventDispatcher.class));
    UITransfer transferUI = createTransferUI(context);

    transferUI.setTransferId("first");
    verify(lynxView, never())
        .dispatchTransferCreate(eq("first"), same(transferUI), any(View.class));

    transferUI.onNodeReady();
    transferUI.onNodeReady();
    transferUI.setTransferId("first");
    transferUI.setTransferId("second");

    ArgumentCaptor<View> wrapperCaptor = ArgumentCaptor.forClass(View.class);
    InOrder inOrder = inOrder(lynxView);
    inOrder.verify(lynxView).dispatchTransferCreate(
        eq("first"), same(transferUI), wrapperCaptor.capture());
    View wrapper = wrapperCaptor.getValue();
    inOrder.verify(lynxView).dispatchTransferRemove("first", wrapper);
    inOrder.verify(lynxView).dispatchTransferCreate("second", transferUI, wrapper);

    transferUI.onNodeRemoved();
    transferUI.destroy();
    verify(lynxView, times(1)).dispatchTransferRemove("second", wrapper);
  }

  @Test
  public void hostViewForwardsChildrenToWrapper() {
    TestLynxContext context = createLynxContext(
        mock(LynxView.class), mock(EventEmitter.class), mock(TouchEventDispatcher.class));
    UITransfer.TransferHostView hostView = createTransferUI(context).getView();
    View first = new View(context);
    View second = new View(context);
    ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(10, 20);

    hostView.addView(first, 0, params);
    assertNotNull(first.getParent());
    assertNotSame(hostView, first.getParent());

    hostView.addView(second, 0, params);
    ViewGroup wrapper = (ViewGroup) first.getParent();
    assertSame(second, wrapper.getChildAt(0));
    assertSame(first, wrapper.getChildAt(1));

    hostView.removeViewAt(0);
    assertNull(second.getParent());
    assertNotNull(first.getParent());

    hostView.removeView(first);
    assertNull(first.getParent());

    hostView.addView(first, 0, params);
    hostView.addView(second, 1, params);
    hostView.removeAllViews();
    assertNull(first.getParent());
    assertNull(second.getParent());
  }

  @Test
  public void wrapperMeasureUpdatesShadowNodeConstraints() {
    LynxView lynxView = mock(LynxView.class);
    TestLynxContext context =
        createLynxContext(lynxView, mock(EventEmitter.class), mock(TouchEventDispatcher.class));
    TransferShadowNode shadowNode = mock(TransferShadowNode.class);
    context.mShadowNode = shadowNode;
    UITransfer transferUI = createTransferUI(context);
    transferUI.setTransferId("transfer");
    transferUI.onNodeReady();

    ArgumentCaptor<View> wrapperCaptor = ArgumentCaptor.forClass(View.class);
    verify(lynxView).dispatchTransferCreate(
        eq("transfer"), same(transferUI), wrapperCaptor.capture());
    View wrapper = wrapperCaptor.getValue();
    int widthSpec = View.MeasureSpec.makeMeasureSpec(300, View.MeasureSpec.EXACTLY);
    int heightSpec = View.MeasureSpec.makeMeasureSpec(200, View.MeasureSpec.AT_MOST);
    wrapper.measure(widthSpec, heightSpec);

    verify(shadowNode).updateHostConstraints(300, MeasureMode.EXACTLY, 200, MeasureMode.AT_MOST);
    assertEquals(300, wrapper.getMeasuredWidth());
    assertEquals(200, wrapper.getMeasuredHeight());

    wrapper.requestLayout();
    wrapper.measure(widthSpec, heightSpec);
    verify(shadowNode, times(1))
        .updateHostConstraints(300, MeasureMode.EXACTLY, 200, MeasureMode.AT_MOST);
  }

  @Test
  public void transferTimingEventIsSentOnceForEachAttachment() {
    LynxView lynxView = mock(LynxView.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    TestLynxContext context =
        createLynxContext(lynxView, eventEmitter, mock(TouchEventDispatcher.class));
    UITransfer transferUI = createTransferUI(context);
    Map<String, EventsListener> events = new HashMap<>();
    events.put(UITransfer.EVENT_TRANSFER_TIMING,
        new EventsListener(
            UITransfer.EVENT_TRANSFER_TIMING, "bindEvent", "onTransferTiming", null, null));
    transferUI.setEvents(events);
    transferUI.setTransferId("first");
    transferUI.onNodeReady();

    ArgumentCaptor<View> wrapperCaptor = ArgumentCaptor.forClass(View.class);
    verify(lynxView).dispatchTransferCreate(eq("first"), same(transferUI), wrapperCaptor.capture());
    View wrapper = wrapperCaptor.getValue();
    wrapper.layout(0, 0, 10, 10);
    wrapper.draw(new Canvas());
    wrapper.draw(new Canvas());
    transferUI.setTransferId("second");
    wrapper.draw(new Canvas());

    ArgumentCaptor<LynxCustomEvent> eventCaptor = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(eventEmitter, times(2)).sendCustomEvent(eventCaptor.capture());
    List<LynxCustomEvent> capturedEvents = eventCaptor.getAllValues();
    assertTimingEvent(capturedEvents.get(0), "first");
    assertTimingEvent(capturedEvents.get(1), "second");
    assertFalse(capturedEvents.get(0).eventParams().isEmpty());
  }

  @Test
  public void transferTimingEventIsSkippedWhenUnbound() {
    EventEmitter eventEmitter = mock(EventEmitter.class);
    TestLynxContext context =
        createLynxContext(mock(LynxView.class), eventEmitter, mock(TouchEventDispatcher.class));
    UITransfer transferUI = createTransferUI(context);

    transferUI.sendTransferDrawEndEvent();

    verify(eventEmitter, never()).sendCustomEvent(any(LynxCustomEvent.class));
  }

  private static void assertTimingEvent(LynxCustomEvent event, String transferId) {
    assertEquals(SIGN, event.getTag());
    assertEquals(UITransfer.EVENT_TRANSFER_TIMING, event.getName());
    assertEquals(transferId, event.eventParams().get("transfer_id"));
    Object timing = event.eventParams().get("timing");
    assertTrue(timing instanceof Map);
    Object drawEnd = ((Map<?, ?>) timing).get("draw_end");
    assertTrue(drawEnd instanceof Number);
    assertTrue(((Number) drawEnd).doubleValue() >= 0);
  }
}
