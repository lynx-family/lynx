// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.view.View.MeasureSpec;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.scroll.UIScrollView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class KeyboardAvoidingScrollControllerTest {
  private static final int SCROLL_WIDTH = 200;
  private static final int SCROLL_HEIGHT = 200;
  private static final int CONTENT_HEIGHT = 500;

  private LynxContext mContext;
  private TestDelegate mDelegate;
  private KeyboardAvoidingScrollController mController;
  private View mLynxView;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    mDelegate = new TestDelegate();
    mController = new KeyboardAvoidingScrollController(mDelegate);
    mLynxView = new View(mContext);
  }

  @Test
  public void canHandleTargetOnlyForVerticalScrollView() {
    TestScrollFixture verticalFixture = createScrollFixture("vertical");
    KeyboardEvent.KeyboardAvoidingTarget verticalTarget =
        createTarget(verticalFixture.inputOwner, verticalFixture.scrollView, 120, 40, 0);
    assertTrue(mController.canHandleTarget(verticalTarget));

    TestScrollFixture horizontalFixture = createScrollFixture("horizontal");
    KeyboardEvent.KeyboardAvoidingTarget horizontalTarget =
        createTarget(horizontalFixture.inputOwner, horizontalFixture.scrollView, 120, 40, 0);
    assertFalse(mController.canHandleTarget(horizontalTarget));
  }

  @Test
  public void applyAvoidDistanceAddsExtraAndScrollsToKeepInputVisible() {
    TestScrollFixture fixture = createScrollFixture("vertical");
    fixture.scrollView.setScrollTo(0, 80, false);
    KeyboardEvent.KeyboardAvoidingTarget target =
        createTarget(fixture.inputOwner, fixture.scrollView, 520, 40, 0);
    mDelegate.addTarget(target);

    assertTrue(mController.applyAvoidDistance(target, 300, null, false, false, 0));
    flushScrollLayout(fixture.scrollView);

    assertEquals(60, fixture.scrollView.getKeyboardAvoidingContentHeightExtra());
    assertEquals(560, fixture.scrollView.getContentHeight());
    assertEquals(360, fixture.scrollView.getRealScrollY());
  }

  @Test
  public void clearRestoresOriginalOffsetAndContentHeight() {
    TestScrollFixture fixture = createScrollFixture("vertical");
    fixture.scrollView.setScrollTo(0, 80, false);
    KeyboardEvent.KeyboardAvoidingTarget target =
        createTarget(fixture.inputOwner, fixture.scrollView, 520, 40, 0);
    mDelegate.addTarget(target);

    mController.applyAvoidDistance(target, 300, null, false, false, 0);
    mController.clear(false);

    assertEquals(0, fixture.scrollView.getKeyboardAvoidingContentHeightExtra());
    assertEquals(CONTENT_HEIGHT, fixture.scrollView.getContentHeight());
    assertEquals(80, fixture.scrollView.getRealScrollY());
  }

  @Test
  public void applyAvoidDistancePreservesFocusedBottomWhenInputLayoutMoves() {
    TestScrollFixture fixture = createScrollFixture("vertical");
    KeyboardEvent.KeyboardAvoidingTarget target =
        createTarget(fixture.inputOwner, fixture.scrollView, 520, 40, 0);
    mDelegate.addTarget(target);

    mController.applyAvoidDistance(target, 300, null, false, false, 0);
    flushScrollLayout(fixture.scrollView);
    assertEquals(360, fixture.scrollView.getRealScrollY());

    ScrollAwarePositionedView inputView = (ScrollAwarePositionedView) target.getInputView();
    inputView.setContentFrame(0, 540, 100, 40);
    mController.applyAvoidDistance(target, 300, null, false, false, 0);
    flushScrollLayout(fixture.scrollView);

    assertEquals(380, fixture.scrollView.getRealScrollY());
  }

  private TestScrollFixture createScrollFixture(String orientation) {
    UIScrollView scrollUI = new UIScrollView(mContext);
    scrollUI.setScrollOrientation(orientation);
    scrollUI.setLeft(0);
    scrollUI.setTop(0);
    scrollUI.setWidth(SCROLL_WIDTH);
    scrollUI.setHeight(SCROLL_HEIGHT);

    AndroidScrollView scrollView = scrollUI.getView();
    scrollView.setMeasuredSize(SCROLL_WIDTH, CONTENT_HEIGHT);
    scrollView.measure(MeasureSpec.makeMeasureSpec(SCROLL_WIDTH, MeasureSpec.EXACTLY),
        MeasureSpec.makeMeasureSpec(SCROLL_HEIGHT, MeasureSpec.EXACTLY));
    scrollView.layout(0, 0, SCROLL_WIDTH, SCROLL_HEIGHT);

    UIView inputOwner = new UIView(mContext);
    inputOwner.setLeft(0);
    inputOwner.setTop(0);
    inputOwner.setWidth(SCROLL_WIDTH);
    inputOwner.setHeight(40);
    scrollUI.insertChild(inputOwner, 0);

    return new TestScrollFixture(scrollUI, scrollView, inputOwner);
  }

  private void flushScrollLayout(AndroidScrollView scrollView) {
    scrollView.measure(MeasureSpec.makeMeasureSpec(SCROLL_WIDTH, MeasureSpec.EXACTLY),
        MeasureSpec.makeMeasureSpec(SCROLL_HEIGHT, MeasureSpec.EXACTLY));
    scrollView.layout(0, 0, SCROLL_WIDTH, SCROLL_HEIGHT);
    scrollView.getViewTreeObserver().dispatchOnPreDraw();
  }

  private KeyboardEvent.KeyboardAvoidingTarget createTarget(UIView inputOwner,
      AndroidScrollView scrollView, int inputTopInContent, int inputHeight, float spacing) {
    ScrollAwarePositionedView inputView = new ScrollAwarePositionedView(mContext, scrollView);
    inputView.setContentFrame(0, inputTopInContent, 100, inputHeight);
    return new KeyboardEvent.KeyboardAvoidingTarget(
        inputOwner, inputView, mLynxView, true, spacing);
  }

  private static class TestScrollFixture {
    final UIScrollView scrollUI;
    final AndroidScrollView scrollView;
    final UIView inputOwner;

    TestScrollFixture(UIScrollView scrollUI, AndroidScrollView scrollView, UIView inputOwner) {
      this.scrollUI = scrollUI;
      this.scrollView = scrollView;
      this.inputOwner = inputOwner;
    }
  }

  private static class TestDelegate implements KeyboardAvoidingScrollController.Delegate {
    private final List<KeyboardEvent.KeyboardAvoidingTarget> mTargets = new ArrayList<>();
    private KeyboardEvent.KeyboardAvoidingTarget mActiveTarget;

    void addTarget(KeyboardEvent.KeyboardAvoidingTarget target) {
      mTargets.add(target);
      mActiveTarget = target;
    }

    @Override
    public Iterable<KeyboardEvent.KeyboardAvoidingTarget> getTargets() {
      return mTargets;
    }

    @Override
    public KeyboardEvent.KeyboardAvoidingTarget getActiveTarget() {
      return mActiveTarget;
    }

    @Override
    public Activity getActivity() {
      return null;
    }

    @Override
    public int getKeyboardAvoidingScreenBottom(View decorView) {
      return 0;
    }
  }

  private static class ScrollAwarePositionedView extends View {
    private final AndroidScrollView mScrollView;
    private int mContentX;
    private int mContentY;

    ScrollAwarePositionedView(Context context, AndroidScrollView scrollView) {
      super(context);
      mScrollView = scrollView;
    }

    void setContentFrame(int contentX, int contentY, int width, int height) {
      mContentX = contentX;
      mContentY = contentY;
      layout(contentX, contentY, contentX + width, contentY + height);
    }

    @Override
    public void getLocationOnScreen(int[] outLocation) {
      outLocation[0] = mContentX - mScrollView.getRealScrollX();
      outLocation[1] = mContentY - mScrollView.getRealScrollY();
    }
  }
}
