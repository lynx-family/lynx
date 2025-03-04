// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.Assertions;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxPropsHolder;
import com.lynx.tasm.behavior.PaintingContext;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.text.EventTargetSpan;
import com.lynx.tasm.behavior.utils.PropsUpdater;
import com.lynx.tasm.event.EventsListener;
import java.util.ArrayList;
import java.util.Map;

@LynxPropsHolder
public class ShadowNode extends LayoutNode {
  private static final String TAG = "lynx_ShadowNode";

  private @Nullable String mTagName;
  private @Nullable ShadowNode mRootNode;
  private @Nullable ArrayList<ShadowNode> mChildren;
  private @Nullable ShadowNode mParent;
  protected @Nullable LynxContext mContext;
  protected @Nullable ShadowStyle mShadowStyle;

  private boolean mDestroyed;
  protected Map<String, EventsListener> mEvents;
  protected EventTarget.EnableStatus mIgnoreFocus;
  protected EventTarget.EnableStatus mEventThrough;
  protected ReadableMap mDataset = new JavaOnlyMap();
  protected boolean mEnableTouchPseudoPropagation;

  public ShadowNode() {
    mIgnoreFocus = EventTarget.EnableStatus.Undefined;
    mEventThrough = EventTarget.EnableStatus.Undefined;
    mEnableTouchPseudoPropagation = true;
  }

  public boolean isVirtual() {
    return false;
  }

  public final String getTagName() {
    return Assertions.assertNotNull(mTagName);
  }

  public void setEvents(Map<String, EventsListener> events) {
    this.mEvents = events;
  }

  public void addChildAt(ShadowNode child, int i) {
    if (child.getParent() != null) {
      throw new RuntimeException(
          "Tried to add child that already has a parent! Remove it from its parent first.");
    }
    if (mChildren == null) {
      mChildren = new ArrayList<>(4);
    }
    mChildren.add(i, child);
    child.mParent = this;
  }

  public ShadowNode removeChildAt(int i) {
    if (mChildren == null) {
      throw new ArrayIndexOutOfBoundsException(
          "Index " + i + " out of bounds: node has no children");
    }
    ShadowNode removed = mChildren.remove(i);
    removed.mParent = null;
    return removed;
  }

  public final int getChildCount() {
    return mChildren == null ? 0 : mChildren.size();
  }

  public final ShadowNode getChildAt(int i) {
    if (mChildren == null) {
      throw new ArrayIndexOutOfBoundsException(
          "Index " + i + " out of bounds: node has no children");
    }
    return mChildren.get(i);
  }

  public final int indexOf(ShadowNode child) {
    return mChildren == null ? -1 : mChildren.indexOf(child);
  }

  public final void updateProperties(StylesDiffMap props) {
    // Catch the exception while updating props
    try {
      PropsUpdater.updateProps(this, props);
      onAfterUpdateTransaction();
    } catch (Exception e) {
      LLog.e(TAG, "Catch exception for tag: " + getTagName());
      getContext().handleException(e);
    }
  }
  public void onAfterUpdateTransaction() {
    // no-op
  }

  @Deprecated
  public void onCollectExtraUpdates(PaintingContext paintingContext) {}

  public final void setTagName(String tagName) {
    mTagName = tagName;
  }

  public final @Nullable ShadowNode getParent() {
    return mParent;
  }

  public final LynxContext getContext() {
    return Assertions.assertNotNull(mContext);
  }

  public void setContext(LynxContext context) {
    mContext = context;
  }

  public ShadowStyle getShadowStyle() {
    return mShadowStyle;
  }

  @Override
  public String toString() {
    return mTagName;
  }

  // Call this method if node will not be continue used
  @Override
  public final void destroy() {
    mDestroyed = true;
    onDestroy();
    super.destroy();
  }

  public final boolean isDestroyed() {
    return mDestroyed;
  }

  protected void onDestroy() {}

  protected void reportNullError(String errorMsg) {
    if (mContext != null) {
      LynxError error = new LynxError(
          LynxSubErrorCode.E_LAYOUT_PLATFORM_NODE_NULL, errorMsg, "", LynxError.LEVEL_ERROR);
      mContext.handleLynxError(error);
    }
  }

  // TODO use a better way to solve in visible node !!!
  private ShadowNode findNonVirtualNode() {
    if (!isVirtual()) {
      return this;
    }
    ShadowNode temp = this.getParent();
    while (temp != null && temp.isVirtual()) {
      temp = temp.getParent();
    }
    return temp;
  }

  @Override
  public void markDirty() {
    if (mDestroyed) {
      return;
    }
    if (!isVirtual()) {
      super.markDirty();
      return;
    }
    ShadowNode visibleNode = findNonVirtualNode();
    if (visibleNode != null) {
      visibleNode.markDirty();
    }
  }

  @Override
  public boolean isDirty() {
    if (!isVirtual()) {
      return super.isDirty();
    }

    ShadowNode visibleNode = findNonVirtualNode();
    if (visibleNode != null) {
      return visibleNode.isDirty();
    }

    return false;
  }

  @LynxProp(name = PropsConstants.VERTICAL_ALIGN)
  public void setVerticalAlign(@Nullable ReadableArray array) {
    // be compatible with old pages
    if (!mContext.isTextRefactorEnabled()) {
      setVerticalAlignOnShadowNode(array);
    }
  }

  // vertical-align only use on inline-text,inline-image and inline-view
  protected void setVerticalAlignOnShadowNode(@Nullable ReadableArray array) {
    if (mShadowStyle == null) {
      mShadowStyle = new ShadowStyle();
    }
    if (null == array || array.size() < 2) {
      mShadowStyle.verticalAlign = StyleConstants.VERTICAL_ALIGN_DEFAULT;
      mShadowStyle.verticalAlignLength = 0;
    } else {
      mShadowStyle.verticalAlign = array.getInt(0);
      mShadowStyle.verticalAlignLength = (float) array.getDouble(1);
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.IGNORE_FOCUS)
  public void setIgnoreFocus(@Nullable Dynamic ignoreFocus) {
    // If ignoreFocus is null or not boolean, the mIgnoreFocus will be Undefined.
    if (ignoreFocus == null) {
      mIgnoreFocus = EventTarget.EnableStatus.Undefined;
      return;
    }
    try {
      mIgnoreFocus = ignoreFocus.asBoolean() ? EventTarget.EnableStatus.Enable
                                             : EventTarget.EnableStatus.Disable;
    } catch (Throwable e) {
      LLog.i(TAG, e.toString());
      mIgnoreFocus = EventTarget.EnableStatus.Undefined;
    }
  }

  @LynxProp(name = PropsConstants.EVENT_THROUGH)
  public void setEventThrough(Dynamic eventThrough) {
    // If eventThrough is null or not boolean, the mEventThrough will be Undefined.
    if (eventThrough == null) {
      this.mEventThrough = EventTarget.EnableStatus.Undefined;
    }
    try {
      this.mEventThrough = eventThrough.asBoolean() ? EventTarget.EnableStatus.Enable
                                                    : EventTarget.EnableStatus.Disable;
    } catch (Throwable e) {
      LLog.i(TAG, e.toString());
      this.mEventThrough = EventTarget.EnableStatus.Undefined;
    }
  }

  @LynxProp(name = PropsConstants.DATASET)
  public void setDataset(@Nullable ReadableMap dataset) {
    mDataset = dataset;
  }

  public boolean needGenerateEventTargetSpan() {
    return (this.mEvents != null && !this.mEvents.isEmpty())
        || this.mIgnoreFocus != EventTarget.EnableStatus.Undefined
        || this.mEventThrough != EventTarget.EnableStatus.Undefined;
  }

  public EventTargetSpan toEventTargetSpan() {
    return new EventTargetSpan(getSignature(), this.mEvents, this.mIgnoreFocus,
        this.mEnableTouchPseudoPropagation, this.mEventThrough, this.mDataset);
  }

  @LynxProp(name = PropsConstants.ENABLE_TOUCH_PSEUDO_PROPAGATION)
  public void setEventThroughPropagation(Dynamic enableTouchPseudoPropagation) {
    // If enableTouchPseudoPropagation is null or not boolean, the mEnableTouchPseudoPropagation
    // will be true.
    if (enableTouchPseudoPropagation == null) {
      this.mEnableTouchPseudoPropagation = true;
      return;
    }
    try {
      this.mEnableTouchPseudoPropagation = enableTouchPseudoPropagation.asBoolean();
    } catch (Throwable e) {
      LLog.i(TAG, e.toString());
      this.mEnableTouchPseudoPropagation = true;
    }
  }

  // subclass need to override this function if need to pass custom bundle from ShadowNode to LynxUI
  public @Nullable Object getExtraBundle() {
    return null;
  }

  public boolean supportInlineView() {
    return false;
  }
}
