// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.util.DisplayMetrics;
import android.util.SparseArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.behavior.shadow.LayoutTick;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.event.EventsListener;

/**
 * The shadow node owner manages the hierarchy and layout info of shadow nodes.
 * And will turn shadow node into lynx ui indirectly through ui operation which
 * is the shadow node owner's operation.
 */
public class ShadowNodeOwner extends LayoutContext {
  private LynxContext mLynxContext;
  private final LayoutTick mLayoutTick;
  private final BehaviorRegistry mBehaviorRegistry;
  private final ShadowNodeRegistry mShadowNodeRegistry;

  protected LayoutNodeManager mLayoutNodeManager;
  public ShadowNodeOwner(
      LynxContext context, BehaviorRegistry behaviorRegistry, LayoutTick layoutTick) {
    mLynxContext = context;
    mShadowNodeRegistry = new ShadowNodeRegistry();
    mBehaviorRegistry = behaviorRegistry;
    mLayoutTick = layoutTick;
    mLayoutNodeManager = new LayoutNodeManager();
    createNativeLayoutContext(this);
  }

  @Override
  public void detachNativePtr() {
    super.detachNativePtr();
    if (mShadowNodeRegistry != null) {
      SparseArray<ShadowNode> nodes = mShadowNodeRegistry.getAllNodes();
      if (nodes != null && nodes.size() > 0) {
        for (int i = 0; i < nodes.size(); i++) {
          nodes.valueAt(i).destroy();
        }
      }
    }
  }

  @Override
  public int createNode(int signature, String tagName, ReadableMap props, ReadableMapBuffer styles,
      ReadableArray eventListener, boolean allowInline) {
    Behavior viewManager = mBehaviorRegistry.get(tagName);
    ShadowNode cssNode = viewManager.createShadowNode();
    int shadowNodeType = 0;
    if (cssNode != null) {
      shadowNodeType |= ShadowNodeType.CUSTOM;
    } else {
      shadowNodeType |= ShadowNodeType.COMMON;
      if (allowInline) {
        cssNode = new NativeLayoutNodeRef();
      } else {
        return shadowNodeType;
      }
    }

    cssNode.setSignature(signature);
    cssNode.setTagName(tagName);
    cssNode.setContext(mLynxContext);
    cssNode.setLayoutNodeManager(mLayoutNodeManager);
    cssNode.setEvents(EventsListener.convertEventListeners(eventListener));
    mShadowNodeRegistry.addNode(cssNode);

    StylesDiffMap stylesDiffMap;
    if (props != null) {
      stylesDiffMap = new StylesDiffMap(props, styles);
      cssNode.updateProperties(stylesDiffMap);
    }

    if (!isDestroyed()) {
      long deprecatedNativePtr = 1L;
      cssNode.attachNativePtr(deprecatedNativePtr);
    }

    if (cssNode.isVirtual()) {
      shadowNodeType |= ShadowNodeType.VIRTUAL;
    }
    if (allowInline && cssNode.supportInlineView()) {
      shadowNodeType |= ShadowNodeType.INLINE;
    }
    return shadowNodeType;
  }

  @Override
  public void removeNode(int parentSignature, int childSignature, int index) {
    ShadowNode parentNode = mShadowNodeRegistry.getNode(parentSignature);
    parentNode.removeChildAt(index);
  }

  @Override
  public void insertNode(int parentSignature, int childSignature, int index) {
    ShadowNode parentNode = mShadowNodeRegistry.getNode(parentSignature);
    ShadowNode childNode = mShadowNodeRegistry.getNode(childSignature);
    if (index == -1) {
      index = parentNode.getChildCount();
    }
    parentNode.addChildAt(childNode, index);
    parentNode.markDirty();
  }

  @Override
  public void moveNode(int parentSignature, int childSignature, int fromIndex, int toIndex) {
    ShadowNode parentNode = mShadowNodeRegistry.getNode(parentSignature);
    ShadowNode childNode = mShadowNodeRegistry.getNode(childSignature);
    parentNode.removeChildAt(fromIndex);
    parentNode.addChildAt(childNode, toIndex);
  }

  @Override
  public void destroyNodes(int[] signatures) {
    // Call child destroy as it will not be continue used
    for (int sign : signatures) {
      ShadowNode node = mShadowNodeRegistry.removeNode(sign);
      if (node != null) {
        node.destroy();
      }
    }
  }

  @Override
  public void dispatchOnLayoutBefore(int signature) {
    ShadowNode node = mShadowNodeRegistry.getNode(signature);
    node.onLayoutBefore();
  }

  @Override
  public void dispatchOnLayout(int sign, int left, int top, int width, int height) {
    ShadowNode node = mShadowNodeRegistry.getNode(sign);
    node.onLayout(left, top, width, height);
  }

  @Override
  public void updateProps(
      int signature, ReadableMap props, ReadableMapBuffer styles, ReadableArray eventListeners) {
    ShadowNode cssNode = mShadowNodeRegistry.getNode(signature);
    if (cssNode == null) {
      throw new RuntimeException("Trying to update non-existent view with tag " + signature);
    }

    if (props != null) {
      StylesDiffMap stylesDiffMap = new StylesDiffMap(props, styles);
      cssNode.updateProperties(stylesDiffMap);
    }

    if (eventListeners != null) {
      cssNode.setEvents(EventsListener.convertEventListeners(eventListeners));
    }
  }

  @Override
  public void setFontFaces(ReadableMap props) {
    mLynxContext.setFontFaces(props.getMap("fontfaces"));
  }

  @Override
  public Object getExtraBundle(int signature) {
    ShadowNode node = mShadowNodeRegistry.getNode(signature);
    if (node == null) {
      // ShadowNode not created yet or is already destroied
      return null;
    }
    return node.getExtraBundle();
  }

  @Override
  public void attachLayoutNodeManager(long nativeLayoutNodeManagerPtr) {
    mLayoutNodeManager.attachNativePtr(nativeLayoutNodeManagerPtr);
  }

  public DisplayMetrics getScreenMetrics() {
    return mLynxContext.getScreenMetrics();
  }

  private void requestRelayout() {
    mLayoutTick.request(new Runnable() {
      @Override
      public void run() {
        triggerLayout();
      }
    });
  }

  @Override
  protected void scheduleLayout() {
    if (isDestroyed()) {
      return;
    }
    requestRelayout();
  }

  public ShadowNode getShadowNode(int signature) {
    return mShadowNodeRegistry.getNode(signature);
  }
}
