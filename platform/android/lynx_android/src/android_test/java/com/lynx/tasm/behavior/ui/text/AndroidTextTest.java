// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Canvas;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.view.ActionMode;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.service.ILynxTextService.Page;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.testing.base.TestingUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class AndroidTextTest {
  private AndroidText mAndroidText;
  private int mTextHeight = 0;
  private ILynxTrailService mOriginalTrailService;
  private ILynxTrailService mTrailService;
  private static final class FakeTextServicePage implements Page {
    private static final int TEXT_LENGTH = 30;
    private static final int CHARS_PER_LINE = 10;
    private static final float CHAR_WIDTH = 10.f;
    private static final float LINE_HEIGHT = 20.f;
    private int mSelectionRectsCallCount;

    @Override
    public void drawPageCanvas(Canvas canvas, Drawable.Callback callback) {}

    @Override
    public int getSelectionCharIndex(float touchX, float touchY) {
      int line = Math.max(0, (int) (touchY / LINE_HEIGHT));
      int column = Math.max(0, (int) (touchX / CHAR_WIDTH));
      return Math.min(TEXT_LENGTH, line * CHARS_PER_LINE + column);
    }

    @Override
    public float[] getSelectionRects(int start, int end) {
      mSelectionRectsCallCount++;
      if (start < 0 || end < start || start >= TEXT_LENGTH || start == end) {
        return new float[0];
      }
      end = Math.min(end, TEXT_LENGTH);
      ArrayList<Float> rects = new ArrayList<>();
      int current = start;
      while (current < end) {
        int line = current / CHARS_PER_LINE;
        int lineEnd = Math.min(end, (line + 1) * CHARS_PER_LINE);
        float left = (current - line * CHARS_PER_LINE) * CHAR_WIDTH;
        float top = line * LINE_HEIGHT;
        rects.add(left);
        rects.add(top);
        rects.add((lineEnd - current) * CHAR_WIDTH);
        rects.add(LINE_HEIGHT);
        current = lineEnd;
      }
      float[] result = new float[rects.size()];
      for (int i = 0; i < rects.size(); i++) {
        result[i] = rects.get(i);
      }
      return result;
    }

    @Override
    public int getTextLength() {
      return TEXT_LENGTH;
    }

    @Override
    public String getSelectedText(int start, int end) {
      String text = "abcdefghijklmnopqrstuvwxyz1234";
      return text.substring(start, end);
    }

    @Override
    public void destroy() {}

    int getSelectionRectsCallCount() {
      return mSelectionRectsCallCount;
    }
  }

  @Before
  public void setUp() throws Exception {
    LynxContext lynxContext = TestingUtils.getLynxContext();
    mAndroidText = new AndroidText(lynxContext);

    Layout layout = buildTextLayout(
        "This is a test text.This is a test text.This is a test text.This is a test text.", 200);
    mTextHeight = layout.getHeight();
    mAndroidText.setRight(200);
    mAndroidText.setBottom(mTextHeight);
    TextUpdateBundle textUpdateBundle = new TextUpdateBundle(layout, false, new HashSet<>(), false);
    textUpdateBundle.setTextTranslateOffset(new PointF());
    mAndroidText.setTextBundle(textUpdateBundle);
  }

  private Layout buildTextLayout(String text, int width) {
    SpannableStringBuilder span = new SpannableStringBuilder(text);
    TextPaint textPaint = new TextPaint();
    textPaint.setTextSize(30);
    return new StaticLayout(span, textPaint, width, Layout.Alignment.ALIGN_NORMAL, 1, 0, false);
  }

  @After
  public void tearDown() throws Exception {
    if (mTrailService != null) {
      if (mOriginalTrailService != null) {
        LynxServiceCenter.inst().registerService(mOriginalTrailService);
      } else {
        LynxServiceCenter.inst().unregisterService(ILynxTrailService.class);
      }
      LynxEnv.inst().setSettings(new HashMap<>());
    }
    mAndroidText = null;
  }

  private void setDismissMenuSetting(String value) {
    if (mTrailService == null) {
      LynxServiceCenter.inst().initialize(ApplicationProvider.getApplicationContext());
      mOriginalTrailService = LynxServiceCenter.inst().getService(ILynxTrailService.class);
      mTrailService = mock(ILynxTrailService.class);
      doReturn(ILynxTrailService.class).when(mTrailService).getServiceClass();
      LynxServiceCenter.inst().registerService(mTrailService);
    }
    when(mTrailService.stringValueForTrailKey(
             LynxEnvKey.ENABLE_DISMISS_TEXT_SELECTION_MENU_ON_BACKGROUND.getDescription()))
        .thenReturn(value);
    LynxEnv.inst().setSettings(new HashMap<>());
  }

  private void notifyBackground() {
    mAndroidText.hideToolbarOnBackground();
  }

  private LynxUIOwner createUIOwner() {
    LynxContext context = TestingUtils.getLynxContext();
    LynxUIOwner owner = spy(
        TestingUtils.getLynxUIOwner(context, TestingUtils.getUIBody(context).getBodyView(), null));
    context.setLynxUIOwner(owner);
    return owner;
  }

  private UIText createTextUI(LynxUIOwner owner, int sign, StylesDiffMap props) {
    owner.createViewInternal(sign, "text", props, null, false, 0, null);
    LynxBaseUI node = owner.getNode(sign);
    UIText text =
        node instanceof UIShadowProxy ? (UIText) ((UIShadowProxy) node).getChild() : (UIText) node;
    TextUpdateBundle bundle =
        new TextUpdateBundle(mAndroidText.mTextLayout, false, new HashSet<>(), false);
    bundle.setTextTranslateOffset(new PointF());
    text.getView().setTextBundle(bundle);
    return text;
  }

  @Test
  public void testBackgroundHidesToolbarWithoutClearingSelection() {
    setDismissMenuSetting(null);
    mAndroidText.setTextSelection(1, 10, 10, 10, true, true);
    String selectedText = mAndroidText.getSelectedText();
    ActionMode mode = mock(ActionMode.class);
    mAndroidText.onPrepareActionMode(mode, null);

    notifyBackground();
    notifyBackground();

    verify(mode, times(1)).finish();
    assertEquals(selectedText, mAndroidText.getSelectedText());
    assertEquals(2, mAndroidText.getHandlesInfo().length);
  }

  @Test
  public void testBackgroundToolbarSettingCanRollBackAndUpdate() {
    setDismissMenuSetting("false");
    mAndroidText.setTextSelection(1, 10, 10, 10, true, true);
    ActionMode mode = mock(ActionMode.class);
    mAndroidText.onPrepareActionMode(mode, null);

    notifyBackground();
    verify(mode, never()).finish();

    setDismissMenuSetting("true");
    notifyBackground();
    verify(mode).finish();
  }

  @Test
  public void testBackgroundPreservesCustomMenuSelectionWithoutSystemToolbar() {
    setDismissMenuSetting("true");
    mAndroidText.setCustomContextMenu(true);
    mAndroidText.setTextSelection(1, 10, 10, 10, true, true);
    String selectedText = mAndroidText.getSelectedText();

    notifyBackground();

    assertEquals(selectedText, mAndroidText.getSelectedText());
    assertEquals(2, mAndroidText.getHandlesInfo().length);
  }

  @Test
  public void testBackgroundHidesSystemToolbarAfterEnablingCustomMenu() {
    setDismissMenuSetting("true");
    mAndroidText.setTextSelection(1, 10, 10, 10, true, true);
    String selectedText = mAndroidText.getSelectedText();
    ActionMode mode = mock(ActionMode.class);
    mAndroidText.onPrepareActionMode(mode, null);
    mAndroidText.setCustomContextMenu(true);

    notifyBackground();
    notifyBackground();

    verify(mode, times(1)).finish();
    assertEquals(selectedText, mAndroidText.getSelectedText());
    assertEquals(2, mAndroidText.getHandlesInfo().length);
  }

  @Test
  public void testBackgroundPreservesTextServiceSelection() {
    setDismissMenuSetting("true");
    mAndroidText.setTextBundle(new FakeTextServicePage());
    mAndroidText.setTextSelection(5, 5, 35, 5, true, true);
    ActionMode mode = mock(ActionMode.class);
    mAndroidText.onPrepareActionMode(mode, null);

    notifyBackground();

    verify(mode).finish();
    assertEquals("abc", mAndroidText.getSelectedText());
    assertEquals(2, mAndroidText.getHandlesInfo().length);
  }

  @Test
  public void testOtherLynxViewBackgroundPreservesToolbar() {
    setDismissMenuSetting("true");
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxUIOwner owner = createUIOwner();
      UIText text = createTextUI(owner, 1, null);
      text.setEnableTextSelection(true);
      text.getView().setTextSelection(1, 10, 10, 10, true, true);
      ActionMode mode = mock(ActionMode.class);
      text.getView().onPrepareActionMode(mode, null);

      createUIOwner().onEnterBackground();
      verify(mode, never()).finish();
      owner.onEnterBackground();
      verify(mode).finish();
    });
  }

  @Test
  public void testUIOwnerBackgroundHidesSelectedTextToolbar() {
    setDismissMenuSetting("true");
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxUIOwner owner = createUIOwner();
      JavaOnlyMap props = new JavaOnlyMap();
      props.putBoolean("text-selection", true);
      UIText text = createTextUI(owner, 1, new StylesDiffMap(props));
      text.getView().setTextSelection(1, 10, 10, 10, true, true);
      ActionMode mode = mock(ActionMode.class);
      text.getView().onPrepareActionMode(mode, null);

      owner.onEnterForeground();
      verify(mode, never()).finish();
      owner.onEnterBackground();
      verify(mode).finish();
    });
  }

  @Test
  public void testBackgroundHidesProgrammaticSelectionToolbar() {
    setDismissMenuSetting("true");
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxUIOwner owner = createUIOwner();
      UIText text = createTextUI(owner, 1, null);
      Callback callback = mock(Callback.class);
      JavaOnlyMap params = new JavaOnlyMap();
      params.putDouble("startX", 1);
      params.putDouble("startY", 3);
      params.putDouble("endX", 10);
      params.putDouble("endY", 3);
      text.setTextSelection(params, callback);
      verify(callback).invoke(eq(LynxUIMethodConstants.SUCCESS), any(JavaOnlyMap.class));
      text.setTextSelection(params, callback);
      ActionMode mode = mock(ActionMode.class);
      text.getView().onPrepareActionMode(mode, null);

      owner.onEnterBackground();

      verify(mode).finish();
    });
  }

  @Test
  public void testDestroyedTextUnregistersBackgroundListener() {
    setDismissMenuSetting("true");
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxUIOwner owner = createUIOwner();
      UIText text = createTextUI(owner, 1, null);
      text.setEnableTextSelection(true);
      ActionMode mode = mock(ActionMode.class);
      text.getView().onPrepareActionMode(mode, null);

      owner.destroy(-1, 1);
      verify(owner).unregisterForegroundListener(text);
      owner.onEnterBackground();

      verify(mode, never()).finish();
    });
  }

  @Test
  public void testDestroyedShadowedTextUnregistersBackgroundListener() {
    setDismissMenuSetting("true");
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxUIOwner owner = createUIOwner();
      JavaOnlyMap props = new JavaOnlyMap();
      props.putDouble(PropsConstants.OUTLINE_WIDTH, 1);
      props.putBoolean("text-selection", true);
      UIText text = createTextUI(owner, 1, new StylesDiffMap(props));
      assertTrue(owner.getNode(1) instanceof UIShadowProxy);
      ActionMode mode = mock(ActionMode.class);
      text.getView().onPrepareActionMode(mode, null);

      owner.destroy(-1, 1);

      verify(owner).unregisterForegroundListener(text);
      assertNull(owner.getNode(1));
      owner.onEnterBackground();
      verify(mode, never()).finish();
    });
  }

  private boolean hasRetainedReference(ArrayList<WeakReference<?>> references) {
    for (WeakReference<?> reference : references) {
      if (reference.get() != null) {
        return true;
      }
    }
    return false;
  }

  private void assertCollected(ArrayList<WeakReference<?>> references) throws Exception {
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    for (int attempt = 0; attempt < 30 && hasRetainedReference(references); attempt++) {
      System.gc();
      System.runFinalization();
      Thread.sleep(100);
    }
    assertFalse("Text lifecycle references were retained", hasRetainedReference(references));
  }

  @Test
  public void testUnreferencedPageIsCollectedWithoutDestroy() throws Exception {
    ArrayList<WeakReference<?>> references = new ArrayList<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext context = TestingUtils.getLynxContext();
      // A spy would retain component arguments in its invocation history.
      LynxUIOwner owner =
          TestingUtils.getLynxUIOwner(context, TestingUtils.getUIBody(context).getBodyView(), null);
      context.setLynxUIOwner(owner);
      references.add(new WeakReference<>(owner));
      for (int i = 1; i <= 20; i++) {
        UIText text = createTextUI(owner, i, null);
        text.setEnableTextSelection(true);
        references.add(new WeakReference<>(text));
        references.add(new WeakReference<>(text.getView()));
      }
    });
    assertCollected(references);
  }

  @Test
  public void testDestroyedTextAndShadowProxyAreCollectedWhileOwnerIsAlive() throws Exception {
    LynxUIOwner[] owners = new LynxUIOwner[1];
    ArrayList<WeakReference<?>> references = new ArrayList<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext context = TestingUtils.getLynxContext();
      owners[0] =
          TestingUtils.getLynxUIOwner(context, TestingUtils.getUIBody(context).getBodyView(), null);
      context.setLynxUIOwner(owners[0]);
      for (int i = 1; i <= 20; i++) {
        JavaOnlyMap props = new JavaOnlyMap();
        if (i % 2 == 0) {
          props.putDouble(PropsConstants.OUTLINE_WIDTH, 1);
        }
        UIText text = createTextUI(owners[0], i, new StylesDiffMap(props));
        if (i % 2 == 0) {
          assertTrue(owners[0].getNode(i) instanceof UIShadowProxy);
          references.add(new WeakReference<>(owners[0].getNode(i)));
        }
        text.setEnableTextSelection(true);
        references.add(new WeakReference<>(text));
        references.add(new WeakReference<>(text.getView()));
        owners[0].destroy(-1, i);
      }
    });
    assertCollected(references);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      owners[0].onEnterForeground();
      owners[0].onEnterBackground();
    });
  }

  @Test
  public void testAsyncCreatedTextReceivesBackground() throws Exception {
    setDismissMenuSetting("true");
    LynxUIOwner[] owners = new LynxUIOwner[1];
    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> { owners[0] = createUIOwner(); });
    JavaOnlyMap props = new JavaOnlyMap();
    props.putBoolean("text-selection", true);
    // Prepare on the test thread, then publish through the existing UI-thread completion.
    Runnable completion =
        owners[0].createViewAsyncRunnable(1, "text", props, null, null, false, 0, null);
    assertNotNull(completion);
    assertNull(owners[0].getNode(1));
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      completion.run();
      UIText text = (UIText) owners[0].getNode(1);
      assertNotNull(text);
      ActionMode mode = mock(ActionMode.class);
      text.getView().onPrepareActionMode(mode, null);
      owners[0].onEnterBackground();
      verify(mode).finish();
    });
  }

  @Test
  public void testGetTextBoundingBoxes() {
    ArrayList<RectF> rectFArrayList = mAndroidText.getTextBoundingBoxes(0, 1);
    assertTrue(rectFArrayList.size() == 1);
    RectF rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.top == 0 && rect.right > 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(-1, 1);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(0, 100);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(10, 10);
    assertTrue(rectFArrayList.size() == 1);
    rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.right == 0 && rect.top == 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(2, 40);
    assertTrue(rectFArrayList.size() > 0);
    rect = rectFArrayList.get(0);
    assertTrue(rect.left > 0 && rect.right > rect.left);
  }

  @Test
  public void testSetTextSelection() {
    ArrayList<RectF> rectFArrayList = mAndroidText.setTextSelection(1, 10, 10, 10, false, false);
    assertTrue(rectFArrayList.size() == 1);
    RectF rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.top == 0 && rect.right > 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.setTextSelection(-1, 10, 10, 10, false, false);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.setTextSelection(2, 10, 100, 50, false, false);
    assertTrue(rectFArrayList.size() > 0);
    rect = rectFArrayList.get(0);
    assertTrue(rect.left >= 0 && rect.right > 0 && rect.top == 0 && rect.bottom > 0);
    rect = rectFArrayList.get(1);
    assertTrue(rect.left == 0 && rect.right > 0 && rect.top > 0 && rect.bottom > rect.top);
  }

  @Test
  public void testGetHandlesInfo() {
    ArrayList<Float>[] handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 0);

    mAndroidText.setTextSelection(1, 10, 10, 10, false, false);
    handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 2);
    assertTrue(handles[1].get(0) > handles[0].get(0));
    assertTrue(handles[0].get(1).equals(handles[1].get(1)));
    assertTrue(handles[0].get(2).equals(handles[1].get(2)));

    mAndroidText.setTextSelection(2, 10, 100, 50, false, false);
    handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 2);
    assertTrue(handles[1].get(1).floatValue() > handles[0].get(1).floatValue());
  }

  @Test
  public void testGetSelectedText() {
    String selectedText = mAndroidText.getSelectedText();
    assertTrue(selectedText.isEmpty());

    mAndroidText.setTextSelection(1, 10, 10, 10, false, false);
    selectedText = mAndroidText.getSelectedText();
    assertTrue(selectedText.length() > 0);
  }

  @Test
  public void testTextServiceGetTextBoundingBoxes() {
    mAndroidText.setTextBundle(new FakeTextServicePage());
    ArrayList<RectF> rectFArrayList = mAndroidText.getTextBoundingBoxes(2, 14);
    assertEquals(2, rectFArrayList.size());

    RectF firstLine = rectFArrayList.get(0);
    assertEquals(20.f, firstLine.left, 0.f);
    assertEquals(0.f, firstLine.top, 0.f);
    assertEquals(100.f, firstLine.right, 0.f);
    assertEquals(20.f, firstLine.bottom, 0.f);

    RectF secondLine = rectFArrayList.get(1);
    assertEquals(0.f, secondLine.left, 0.f);
    assertEquals(20.f, secondLine.top, 0.f);
    assertEquals(40.f, secondLine.right, 0.f);
    assertEquals(40.f, secondLine.bottom, 0.f);
  }

  @Test
  public void testTextServiceSetTextSelection() {
    FakeTextServicePage page = new FakeTextServicePage();
    mAndroidText.setTextBundle(page);
    ArrayList<RectF> rectFArrayList = mAndroidText.setTextSelection(5, 5, 35, 5, false, false);
    assertEquals(1, rectFArrayList.size());
    assertEquals(0.f, rectFArrayList.get(0).left, 0.f);
    assertEquals(30.f, rectFArrayList.get(0).right, 0.f);
    assertEquals(1, page.getSelectionRectsCallCount());

    ArrayList<Float>[] handles = mAndroidText.getHandlesInfo();
    assertEquals(2, handles.length);
    assertTrue(handles[1].get(0) > handles[0].get(0));

    assertEquals("abc", mAndroidText.getSelectedText());

    rectFArrayList = mAndroidText.setTextSelection(5, 5, 5, 5, false, false);
    assertEquals(1, rectFArrayList.size());
    assertEquals(10.f, rectFArrayList.get(0).width(), 0.f);
  }

  @Test
  public void testTextServiceSelectionRectCache() {
    FakeTextServicePage page = new FakeTextServicePage();
    mAndroidText.setTextBundle(page);

    mAndroidText.setTextSelection(5, 5, 35, 5, false, false);
    assertEquals(1, page.getSelectionRectsCallCount());

    mAndroidText.getTextBoundingBoxes(0, 3);
    assertEquals(1, page.getSelectionRectsCallCount());

    mAndroidText.getTextBoundingBoxes(0, 4);
    assertEquals(2, page.getSelectionRectsCallCount());
  }

  @Test
  public void testDisplayNoneSkipsDraw() {
    Layout layoutSpy = spy(mAndroidText.mTextLayout);
    TextUpdateBundle textUpdateBundle =
        new TextUpdateBundle(layoutSpy, false, new HashSet<>(), false);
    textUpdateBundle.setTextTranslateOffset(new PointF());
    mAndroidText.setTextBundle(textUpdateBundle);

    mAndroidText.setDisplayNone(true);
    mAndroidText.onDraw(new Canvas());
    verify(layoutSpy, never()).draw(any(Canvas.class));

    mAndroidText.setDisplayNone(false);
    mAndroidText.onDraw(new Canvas());
    verify(layoutSpy).draw(any(Canvas.class));
  }
}
