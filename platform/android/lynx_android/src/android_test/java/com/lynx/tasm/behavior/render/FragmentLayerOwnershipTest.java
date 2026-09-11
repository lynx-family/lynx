// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.BuiltInBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class FragmentLayerOwnershipTest {
  private LynxContext mContext;
  private LynxUIOwner mOwner;
  private PlatformRendererContext mRenderer;

  @Before
  public void setUp() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mContext = TestingUtils.getLynxContext();
      BehaviorRegistry registry = new BehaviorRegistry(new BuiltInBehavior().create());
      registry.addBehavior(new Behavior("layer-content") {
        @Override
        public boolean supportFragmentLayerChildren() {
          return true;
        }

        @Override
        public LynxUI createUI(LynxContext context) {
          return new UIView(context);
        }
      });
      registry.addBehavior(new Behavior("legacy-content") {
        @Override
        public LynxUI createUI(LynxContext context) {
          return new UIView(context);
        }
      });
      UIBody.UIBodyView rootView = new UIBody.UIBodyView(mContext);
      mOwner = new LynxUIOwner(mContext, registry, rootView);
      mContext.setLynxUIOwner(mOwner);
      mRenderer = new PlatformRendererContext(rootView, mContext, registry);
      mRenderer.createPlatformRenderer(1, PlatformRendererContext.PlatformRendererType.kPage);
      mRenderer.createPlatformRenderer(2, PlatformRendererContext.PlatformRendererType.kView);
      mRenderer.createPlatformRenderer(3, PlatformRendererContext.PlatformRendererType.kView);
    });
  }

  @After
  public void tearDown() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.destroy();
      mOwner.destroy();
    });
  }

  @Test
  public void optedInComponentKeepsItsUIAndMountsOrdinaryLayersWithoutDrawLinks() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.createPlatformExtendedRenderer(4, "layer-content", null);
      UIGroup<?> parent = (UIGroup<?>) mOwner.getNode(4);
      assertEquals(UIView.class, parent.getClass());
      assertTrue(parent.usesFragmentLayerChildOrder());
      assertTrue((mRenderer.getTagInfo("layer-content")
                     & PlatformRendererContext.SUPPORT_FRAGMENT_LAYER_CHILDREN)
          != 0);
      mRenderer.insertPlatformRenderer(4, 2, -1, false);
      mRenderer.insertPlatformRenderer(4, 3, 0, false);
      assertNull(parent.getDrawHead());
      assertNull(mOwner.getNode(2).getDrawParent());
      assertSame(mOwner.getNode(3).getFragmentLayerView(), parent.getView().getChildAt(0));
      mRenderer.removePlatformRendererFromParent(4, 3, false);
      mRenderer.insertPlatformRenderer(4, 3, -1, false);
      assertSame(mOwner.getNode(3).getFragmentLayerView(), parent.getView().getChildAt(1));
      assertNull(parent.getDrawHead());
      mRenderer.destroyPlatformRenderer(4);
      assertFalse(parent.usesFragmentLayerChildOrder());
    });
  }

  @Test
  public void compatibleComponentStillMaintainsChildDrawLinks() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.createPlatformExtendedRenderer(4, "legacy-content", null);
      UIGroup<?> parent = (UIGroup<?>) mOwner.getNode(4);
      assertFalse(parent.usesFragmentLayerChildOrder());
      assertEquals(0,
          mRenderer.getTagInfo("legacy-content")
              & PlatformRendererContext.SUPPORT_FRAGMENT_LAYER_CHILDREN);
      mRenderer.insertPlatformRenderer(4, 2, -1, true);
      assertSame(mOwner.getNode(2), parent.getDrawHead());
      assertSame(parent, mOwner.getNode(2).getDrawParent());
    });
  }

  @Test
  public void optedInComponentStillDrawsCompatibleFlattenChildren() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.createPlatformExtendedRenderer(4, "layer-content", null);
      mRenderer.insertPlatformRenderer(4, 2, -1, false);
      LynxFlattenUI flatten = new LynxFlattenUI(mContext);
      flatten.setSign(5, "view");
      mOwner.setNode(5, flatten);
      mOwner.insert(4, 5, 0);
      UIGroup<?> parent = (UIGroup<?>) mOwner.getNode(4);
      assertSame(flatten, parent.getDrawHead());
      assertSame(parent, flatten.getDrawParent());
      assertSame(mOwner.getNode(2), flatten.getNextDrawUI());
      mOwner.remove(4, 5);
      assertNull(flatten.getDrawParent());
      assertEquals(1, parent.getView().getChildCount());
    });
  }

  @Test
  public void optedInFlattenComponentUsesAncestorDrawList() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxFlattenUI flatten = new LynxFlattenUI(mContext);
      flatten.setSign(4, "view");
      flatten.attachFragmentLayer(4, mRenderer, true);
      mOwner.setNode(4, flatten);
      mRenderer.insertPlatformRenderer(1, 4, -1, false);
      mRenderer.insertPlatformRenderer(4, 2, -1, false);
      assertFalse(flatten.usesFragmentLayerChildOrder());
      assertSame(mOwner.getNode(1), mOwner.getNode(2).getDrawParent());
      assertSame(mOwner.getNode(2), flatten.getNextDrawUI());
      assertSame(mOwner.getNode(2).getFragmentLayerView(),
          ((UIGroup<?>) mOwner.getNode(1)).getView().getChildAt(0));
    });
  }

  @Test
  public void ordinaryLayersMountReorderAndRemoveWithoutDrawLinks() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.insertPlatformRenderer(1, 2, -1, false);
      mRenderer.insertPlatformRenderer(1, 3, -1, false);
      UIGroup<?> root = (UIGroup<?>) mOwner.getNode(1);
      assertNull(root.getDrawHead());
      assertNull(mOwner.getNode(2).getDrawParent());
      assertNull(mOwner.getNode(3).getPreviousDrawUI());
      assertSame(mOwner.getNode(2).getFragmentLayerView(), root.getView().getChildAt(0));

      mRenderer.insertPlatformRenderer(1, 3, 0, false);
      assertSame(mOwner.getNode(3), root.getChildAt(0));
      assertSame(mOwner.getNode(3).getFragmentLayerView(), root.getView().getChildAt(0));
      assertEquals(2, root.getView().getChildCount());
      assertNull(root.getDrawHead());

      mRenderer.removePlatformRendererFromParent(1, 3, false);
      assertNull(mOwner.getNode(3).getParentBaseUI());
      assertNull(mOwner.getNode(3).getFragmentLayerView().getParent());
      assertEquals(1, root.getChildren().size());
    });
  }

  @Test
  public void fallbackViewSharesOwnerAndCanMoveBetweenLayers() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      UIView fallback = new UIView(mContext);
      fallback.setSign(4, "view");
      mOwner.setNode(4, fallback);
      mOwner.attachFragmentLayer(4, mRenderer);
      mRenderer.insertPlatformRenderer(1, 2, -1, false);
      mRenderer.insertPlatformRenderer(2, 4, -1, false);
      assertSame(mOwner.getNode(2), fallback.getParentBaseUI());
      assertNull(fallback.getDrawParent());
      mRenderer.insertPlatformRenderer(1, 4, 0, false);
      assertSame(mOwner.getNode(1), fallback.getParentBaseUI());
      assertEquals(0, mOwner.getNode(2).getChildren().size());
      assertSame(fallback.getView(), ((UIGroup<?>) mOwner.getNode(1)).getView().getChildAt(0));
      mRenderer.destroyPlatformRenderer(4);
      assertNull(mOwner.getNode(4));
      assertNull(fallback.getView().getParent());
      assertFalse(fallback.isFragmentLayer());
    });
  }

  @Test
  public void compatibilityFlattenChildMaterializesDrawListForExistingViews() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.insertPlatformRenderer(1, 2, -1, false);
      mRenderer.insertPlatformRenderer(2, 3, -1, false);
      LynxFlattenUI flatten = new LynxFlattenUI(mContext);
      flatten.setSign(4, "view");
      mOwner.setNode(4, flatten);
      mOwner.insert(2, 4, 0);
      LynxUI<?> parent = (LynxUI<?>) mOwner.getNode(2);
      assertSame(flatten, parent.getDrawHead());
      assertSame(parent, flatten.getDrawParent());
      assertSame(mOwner.getNode(3), flatten.getNextDrawUI());
      assertSame(
          mOwner.getNode(3).getFragmentLayerView(), ((UIGroup<?>) parent).getView().getChildAt(0));
      mOwner.remove(2, 4);
      assertSame(mOwner.getNode(3), parent.getDrawHead());
      assertNull(flatten.getDrawParent());
    });
  }

  @Test
  public void layerMountDoesNotReparentOverlay() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxBaseUI overlay = mock(LynxBaseUI.class);
      when(overlay.isOverlay()).thenReturn(true);
      mOwner.setNode(4, overlay);
      mRenderer.insertPlatformRenderer(1, 4, 0, false);
      assertTrue(mOwner.getNode(1).getChildren().isEmpty());
      verify(overlay, never()).setParent(any());
      mOwner.setNode(4, null);
    });
  }

  @Test
  public void rootFrameUpdatesOwnedLayoutUsedByPaintingMetrics() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mRenderer.updatePlatformRendererFrame(
          1, false, 1, 2, 100, 60, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      UIBody root = (UIBody) mOwner.getNode(1);
      assertEquals(4, root.getLeft());
      assertEquals(6, root.getTop());
      assertEquals(100, root.getWidth());
      assertEquals(60, root.getLatestHeight());
    });
  }
}
