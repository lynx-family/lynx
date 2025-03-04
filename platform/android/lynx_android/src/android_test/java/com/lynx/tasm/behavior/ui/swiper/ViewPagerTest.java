// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.swiper;

import static com.lynx.tasm.behavior.ui.swiper.ViewPager.SCROLL_DIRECTION_BEGIN;
import static com.lynx.tasm.behavior.ui.swiper.ViewPager.SCROLL_DIRECTION_END;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

@RunWith(AndroidJUnit4.class)
public class ViewPagerTest {
  private LynxContext mLynxContext;
  private XSwiperUI mSwiperUI;
  private ViewPager mViewPager;

  @Before
  public void setUp() throws Exception {
    mLynxContext = TestingUtils.getLynxContext();
    mSwiperUI = new XSwiperUI(mLynxContext);
    mViewPager = mSwiperUI.getView().getViewPager();
  }

  @Test
  public void testFlingToPosition() {
    ViewPager spyViewPager = spy(mViewPager);
    // (1) check for horizontal
    when(spyViewPager.isVertical()).thenReturn(false);
    when(spyViewPager.getChildCount()).thenReturn(0);
    invokeFlingToPosition(spyViewPager, 100.f, 0.f);
    verify(spyViewPager, Mockito.times(0)).invalidate();
    // (2) check for vertical
    when(spyViewPager.isVertical()).thenReturn(true);
    when(spyViewPager.getChildCount()).thenReturn(0);
    invokeFlingToPosition(spyViewPager, 0.f, 100.f);
    verify(spyViewPager, Mockito.times(0)).invalidate();
  }

  @Test
  public void testFlingToPositionInner() {
    ViewPager spyViewPager = spy(mViewPager);
    // init ViewPager
    int childCount = 5;
    int currentPosition = 4;
    setFiled(spyViewPager, "mTotalCount", childCount);
    spyViewPager.setLoop(true);
    // (1) check for horizontal
    when(spyViewPager.isVertical()).thenReturn(false);
    spyViewPager.setOrientation(SwiperView.ORIENTATION_HORIZONTAL);
    // (1.1) check for LTR with flingToStart = true
    spyViewPager.setIsRTL(false);
    invokeFlingToPositionInner(spyViewPager, currentPosition, true);
    verify(spyViewPager).setCurrentIndex(0, true, SCROLL_DIRECTION_END);
    currentPosition = 3;
    invokeFlingToPositionInner(spyViewPager, currentPosition, true);
    verify(spyViewPager).setCurrentIndex(currentPosition + 1, true, SCROLL_DIRECTION_END);
    // (1.2) check for LTR with flingToStart = false
    currentPosition = 0;
    invokeFlingToPositionInner(spyViewPager, currentPosition, false);
    verify(spyViewPager).setCurrentIndex(childCount - 1, true, SCROLL_DIRECTION_BEGIN);
    currentPosition = 1;
    invokeFlingToPositionInner(spyViewPager, currentPosition, false);
    verify(spyViewPager).setCurrentIndex(currentPosition - 1, true, SCROLL_DIRECTION_BEGIN);
    // (1.3) check for RTL with flingToStart = true
    spyViewPager.setIsRTL(true);
    currentPosition = 0;
    invokeFlingToPositionInner(spyViewPager, currentPosition, true);
    verify(spyViewPager, Mockito.times(2))
        .setCurrentIndex(childCount - 1, true, SCROLL_DIRECTION_BEGIN);
    currentPosition = 1;
    invokeFlingToPositionInner(spyViewPager, currentPosition, true);
    verify(spyViewPager, Mockito.times(2))
        .setCurrentIndex(currentPosition - 1, true, SCROLL_DIRECTION_BEGIN);
    // (1.4) check for RTL with flingToStart = false
    currentPosition = 4;
    invokeFlingToPositionInner(spyViewPager, currentPosition, false);
    verify(spyViewPager, Mockito.times(2)).setCurrentIndex(0, true, SCROLL_DIRECTION_END);
    currentPosition = 3;
    invokeFlingToPositionInner(spyViewPager, currentPosition, false);
    verify(spyViewPager, Mockito.times(2))
        .setCurrentIndex(currentPosition + 1, true, SCROLL_DIRECTION_END);

    // (2) check for vertical
    when(spyViewPager.isVertical()).thenReturn(true);
    // (2.1) check with flingToStart = true
    spyViewPager.setIsRTL(false);
    currentPosition = 4;
    invokeFlingToPositionInner(spyViewPager, currentPosition, true);
    verify(spyViewPager, Mockito.times(3)).setCurrentIndex(0, true, SCROLL_DIRECTION_END);
    currentPosition = 3;
    invokeFlingToPositionInner(spyViewPager, currentPosition, true);
    verify(spyViewPager, Mockito.times(3))
        .setCurrentIndex(currentPosition + 1, true, SCROLL_DIRECTION_END);
    // (2.2) check with flingToStart = false
    currentPosition = 0;
    invokeFlingToPositionInner(spyViewPager, currentPosition, false);
    verify(spyViewPager, Mockito.times(3))
        .setCurrentIndex(childCount - 1, true, SCROLL_DIRECTION_BEGIN);
    currentPosition = 1;
    invokeFlingToPositionInner(spyViewPager, currentPosition, false);
    verify(spyViewPager, Mockito.times(3))
        .setCurrentIndex(currentPosition - 1, true, SCROLL_DIRECTION_BEGIN);
  }

  private void invokeFlingToPosition(ViewPager viewPager, float velX, float velY) {
    try {
      Method method =
          ViewPager.class.getDeclaredMethod("flingToPosition", float.class, float.class);
      if (viewPager != null && method != null) {
        method.setAccessible(true);
        method.invoke(viewPager, velX, velY);
      }
    } catch (NoSuchMethodException e) {
      e.printStackTrace();
    } catch (InvocationTargetException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void invokeFlingToPositionInner(
      ViewPager viewPager, int currentPosition, boolean isFlingToStart) {
    try {
      Method method =
          ViewPager.class.getDeclaredMethod("flingToPositionInner", int.class, boolean.class);
      if (viewPager != null && method != null) {
        method.setAccessible(true);
        method.invoke(viewPager, currentPosition, isFlingToStart);
      }
    } catch (NoSuchMethodException e) {
      e.printStackTrace();
    } catch (InvocationTargetException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void setFiled(ViewPager viewPager, String name, Object value) {
    try {
      Field field = ViewPager.class.getDeclaredField(name);
      if (field != null) {
        field.setAccessible(true);
        field.set(viewPager, value);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }
}
