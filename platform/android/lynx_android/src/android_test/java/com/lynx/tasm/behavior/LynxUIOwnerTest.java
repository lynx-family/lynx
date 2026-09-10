// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import android.graphics.Rect;
import android.os.Build;
import android.util.SparseBooleanArray;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxUIOwnerTest {
  private static final String MEMORY_UI_TAG = "memory-ui";

  static final class MemoryUI extends UIView {
    MemoryUI(LynxContext context) {
      super(context);
    }

    @Override
    public long getMemoryUsageBytes() {
      return 10;
    }
  }

  private static final class MemoryUIBehavior extends Behavior {
    MemoryUIBehavior() {
      super(MEMORY_UI_TAG);
    }

    @Override
    public LynxUI createUI(LynxContext context) {
      return new MemoryUI(context);
    }
  }

  private static int getExternalMemoryCandidateCount(LynxUIOwner uiOwner) {
    try {
      Field field = LynxUIOwner.class.getDeclaredField("mRemovedUICandidateIds");
      field.setAccessible(true);
      return ((SparseBooleanArray) field.get(uiOwner)).size();
    } catch (NoSuchFieldException | IllegalAccessException exception) {
      throw new AssertionError(exception);
    }
  }

  private LynxUI mLynxUI = null;
  private LynxContext mContext;
  private LynxView mLynxView;
  private UIBody mUIBody;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    mLynxUI = mock(LynxUI.class);
    mLynxUI.updateLayout(
        0, 0, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(0, 0, 100, 100));

    LynxUI child = mock(LynxUI.class);
    child.updateLayout(0, 0, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(0, 0, 50, 50));
    mLynxUI.insertChild(child, 0);
  }

  @After
  public void tearDown() throws Exception {
    mLynxUI = null;
  }

  @Test
  public void testCopyUI() {
    // Avoid reporting it in the lower version of Android.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
      return;
    }
    try {
      Class lynxEnvCls = LynxEnv.inst().getClass();
      Method method = lynxEnvCls.getDeclaredMethod("getBehaviors");
      List<Behavior> behaviors = (List<Behavior>) method.invoke(LynxEnv.inst());

      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());
      LynxBaseUI copiedUI = mLynxUI.clone();
      assertEquals(100, copiedUI.getWidth());
      assertEquals(100, copiedUI.getHeight());

      LynxBaseUI copiedChild = copiedUI.getChildAt(0);
      assertEquals(50, copiedChild.getWidth());
      assertEquals(50, copiedChild.getHeight());

    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Test
  public void testGetTagInfo() {
    try {
      List<Behavior> behaviors = new BuiltInBehavior().create();

      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());

      assertEquals(uiOwner.getTagInfo("list"), ShadowNodeType.COMMON);
      assertEquals(uiOwner.getTagInfo("text"), ShadowNodeType.CUSTOM);
      assertEquals(uiOwner.getTagInfo("view"), ShadowNodeType.COMMON);
      assertEquals(uiOwner.getTagInfo("raw-text"), ShadowNodeType.CUSTOM | ShadowNodeType.VIRTUAL);
      assertEquals(
          uiOwner.getTagInfo("inline-text"), ShadowNodeType.CUSTOM | ShadowNodeType.VIRTUAL);
      assertEquals(uiOwner.getTagInfo("frame"), ShadowNodeType.CUSTOM);
      assertEquals(uiOwner.getTagInfo("xxxx"), 0);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testExternalMemoryRemovedIdCandidatesFollowUILifetime() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      List<Behavior> behaviors = new BuiltInBehavior().create();
      behaviors.add(new MemoryUIBehavior());
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());
      contextSpy.setLynxUIOwner(uiOwner);

      uiOwner.createViewInternal(1, LynxConstants.ROOT_TAG_NAME, null, null, false, 0, null);
      uiOwner.createViewInternal(2, MEMORY_UI_TAG, null, null, false, 0, null);
      uiOwner.createViewInternal(3, MEMORY_UI_TAG, null, null, false, 0, null);
      uiOwner.insert(1, 2, 0);
      uiOwner.insert(2, 3, 0);
      uiOwner.cacheRemovedUIIds(new int[] {2, 2, 3});
      long[] snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(20, snapshot[0]);
      assertEquals(0, snapshot[1]);

      uiOwner.remove(1, 2);
      uiOwner.cacheRemovedUIIds(new int[] {2, 2, 3});
      snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(20, snapshot[0]);
      assertEquals(20, snapshot[1]);
      assertEquals(20, uiOwner.getExternalMemorySnapshot()[1]);
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));

      uiOwner.insert(1, 2, 0);
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));
      uiOwner.remove(1, 2);
      assertEquals(20, uiOwner.getExternalMemorySnapshot()[1]);

      uiOwner.createViewInternal(4, MEMORY_UI_TAG, null, null, false, 0, null);
      uiOwner.insert(1, 4, 0);
      uiOwner.cacheRemovedUIIds(new int[] {2, 2, 3});
      uiOwner.remove(1, 4);
      uiOwner.cacheRemovedUIIds(new int[] {2, 4, 4});
      snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(30, snapshot[0]);
      assertEquals(30, snapshot[1]);
      assertEquals(30, uiOwner.getExternalMemorySnapshot()[1]);

      uiOwner.destroy(-1, 4);
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));
      snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(20, snapshot[0]);
      assertEquals(20, snapshot[1]);

      uiOwner.destroy(-1, 2);
      uiOwner.destroy(-1, 3);
      assertEquals(0, getExternalMemoryCandidateCount(uiOwner));
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);
    });
  }

  @Test
  public void testExternalMemorySnapshotPrunesMissingCandidates() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner = new LynxUIOwner(
          contextSpy, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());

      uiOwner.cacheRemovedUIIds(new int[] {404, 405});
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);
      assertEquals(0, getExternalMemoryCandidateCount(uiOwner));
    });
  }

  @Test
  public void testExternalMemoryShadowProxyDelegation() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner = new LynxUIOwner(
          contextSpy, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());
      contextSpy.setLynxUIOwner(uiOwner);

      MemoryUI memoryUI = new MemoryUI(contextSpy);
      memoryUI.setSign(1, MEMORY_UI_TAG);
      MemoryUI descendant = new MemoryUI(contextSpy);
      descendant.setSign(2, MEMORY_UI_TAG);
      memoryUI.insertChild(descendant, 0);
      UIShadowProxy proxy = new UIShadowProxy(contextSpy, memoryUI);
      uiOwner.createViewInternal(100, LynxConstants.ROOT_TAG_NAME, null, null, false, 0, null);
      uiOwner.setNode(1, proxy);
      uiOwner.setNode(2, descendant);
      uiOwner.insert(100, 1, 0);

      assertEquals(20, uiOwner.getExternalMemorySnapshot()[0]);
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);

      uiOwner.remove(100, 1);
      uiOwner.cacheRemovedUIIds(new int[] {1});
      assertEquals(20, uiOwner.getExternalMemorySnapshot()[1]);
    });
  }
}
