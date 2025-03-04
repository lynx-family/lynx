// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.graphics.RectF;
import android.util.DisplayMetrics;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class LynxIntersectionObserver {
  private int mObserverId;
  private WeakReference<LynxIntersectionObserverManager> mManager;
  private LynxBaseUI mAttachedUI;
  private LynxBaseUI mContainer;
  private LynxBaseUI mRoot;

  private float mMarginLeft;
  private float mMarginRight;
  private float mMarginTop;
  private float mMarginBottom;

  private ArrayList<Float> mThresholds;
  private float mInitialRatio;
  private boolean mObserveAll; // TODO: not support now
  private boolean mRelativeToScreen;

  private ArrayList<LynxUIObservationTarget> mObservationTargets;
  private boolean mIsCustomEventObserver;

  private static final String TAG = "LynxIntersectionObserver";

  public LynxIntersectionObserver(LynxIntersectionObserverManager manager, int observerId,
      String componentId, ReadableMap options) {
    mManager = new WeakReference<>(manager);
    mObserverId = observerId;

    if (LynxConstants.LYNX_DEFAULT_COMPONENT_ID.equals(componentId)) {
      mContainer = mManager.get().getContext().getUIBody();
    } else {
      mContainer = mManager.get().getContext().findLynxUIByComponentId(componentId);
    }

    mThresholds = new ArrayList<>();
    ReadableArray thresholds = options.getArray("thresholds");
    if (thresholds != null) {
      for (int i = 0; i < thresholds.size(); i++) {
        mThresholds.add((float) thresholds.getDouble(i));
      }
    } else {
      mThresholds.add(0.f);
    }

    mInitialRatio = (float) options.getDouble("initialRatio", 0.f);
    mObserveAll = options.getBoolean("observeAll", false);

    mObservationTargets = new ArrayList<>();
    mIsCustomEventObserver = false;
  }

  public LynxIntersectionObserver(
      LynxIntersectionObserverManager manager, ReadableMap options, LynxBaseUI attachedUI) {
    this(manager, -1, LynxConstants.LYNX_DEFAULT_COMPONENT_ID, options);
    mAttachedUI = attachedUI;
    String relativeToIdSelector = options.getString("relativeToIdSelector", null);
    mRelativeToScreen = options.getBoolean("relativeToScreen", false);
    mMarginLeft = UnitUtils.toPx(options.getString("marginLeft", "0"));
    mMarginRight = UnitUtils.toPx(options.getString("marginRight", "0"));
    mMarginTop = UnitUtils.toPx(options.getString("marginTop", "0"));
    mMarginBottom = UnitUtils.toPx(options.getString("marginBottom", "0"));
    if (relativeToIdSelector != null && relativeToIdSelector.startsWith("#")) {
      mRoot =
          (LynxBaseUI) mManager.get().getContext().getLynxUIOwner().findLynxUIByIdSelectorSearchUp(
              relativeToIdSelector.substring(1), mAttachedUI);
    }
    mIsCustomEventObserver = true;

    LynxUIObservationTarget observationTarget = new LynxUIObservationTarget();
    observationTarget.ui = attachedUI;
    mObservationTargets.add(observationTarget);

    RectF rootRect = this.getRootRect();
    checkForIntersectionWithTarget(observationTarget, rootRect, true);
  }

  public LynxBaseUI getAttachedUI() {
    return mAttachedUI;
  }

  public int getObserverId() {
    return mObserverId;
  }

  public LynxContext getContext() {
    LynxIntersectionObserverManager manager = mManager.get();
    if (manager == null) {
      LLog.e(TAG, "getContext failed because mManager is null");
      return null;
    }
    return manager.getContext();
  }

  private LynxUIOwner getRootUIOwner() {
    LynxContext context = getContext();
    if (context == null) {
      LLog.e(TAG, "getRootUIOwner failed because context is null");
      return null;
    }
    return context.getLynxUIOwner();
  }

  public void relativeTo(String selector, final ReadableMap margins) {
    if (!selector.startsWith("#"))
      return;
    if (getContext() == null) {
      LLog.e(TAG, "relativeTo failed because context is null");
      mRoot = null;
    } else {
      mRoot = (LynxBaseUI) getContext().findLynxUIByIdSelector(selector.substring(1), mContainer);
    }
    // for historical reason, to avoid break, if not found in container, finding in all element
    if (mRoot == null) {
      LLog.w(TAG, "Can't find element, finding in element");
      if (getRootUIOwner() == null) {
        LLog.e(TAG, "relativeTo failed because UIOwner is null");
      } else {
        mRoot = getRootUIOwner().findLynxUIByIdSelector(selector.substring(1));
      }
    }

    this.parseMargin(margins);
  }

  public void relativeToViewport(final ReadableMap margins) {
    mRoot = null;
    this.parseMargin(margins);
  }

  public void relativeToScreen(final ReadableMap margins) {
    mRoot = null;
    mRelativeToScreen = true;
    this.parseMargin(margins);
  }

  public void observe(String selector, int callbackId) {
    if (!selector.startsWith("#"))
      return;
    LynxBaseUI target = null;
    if (getContext() == null) {
      LLog.e(TAG, "observer failed because context is null");
    } else {
      target = (LynxBaseUI) getContext().findLynxUIByIdSelector(selector.substring(1), mContainer);
    }
    // for historical reason, to avoid break, if not found in container, finding all element
    if (target == null) {
      LLog.w(TAG, "Can't find element, finding in element");
      if (getRootUIOwner() == null) {
        LLog.e(TAG, "observer failed because UIOwner is null");
      } else {
        target = getRootUIOwner().findLynxUIByIdSelector(selector.substring(1));
      }
    }

    if (target != null) {
      for (int i = 0; i < mObservationTargets.size(); i++) {
        if (mObservationTargets.get(i).ui == target) {
          return;
        }
      }
      LynxUIObservationTarget observationTarget = new LynxUIObservationTarget();
      observationTarget.ui = target;
      observationTarget.jsCallbackId = callbackId;
      mObservationTargets.add(observationTarget);

      RectF rootRect = this.getRootRect();
      checkForIntersectionWithTarget(observationTarget, rootRect, true);
    }
  }

  public void disconnect() {
    mObservationTargets.clear();
    mManager.get().removeIntersectionObserver(mObserverId);
  }

  private void parseMargin(final ReadableMap margins) {
    mMarginLeft = UnitUtils.toPx(margins.getString("left", "0"));
    mMarginRight = UnitUtils.toPx(margins.getString("right", "0"));
    mMarginTop = UnitUtils.toPx(margins.getString("top", "0"));
    mMarginBottom = UnitUtils.toPx(margins.getString("bottom", "0"));
  }

  public void checkForIntersections() {
    if (mObservationTargets.size() == 0)
      return;

    RectF rootRect = this.getRootRect();
    for (LynxUIObservationTarget target : mObservationTargets) {
      checkForIntersectionWithTarget(target, rootRect, false);
    }
  }

  private void checkForIntersectionWithTarget(
      LynxUIObservationTarget target, RectF rootRect, boolean isInitial) {
    if (mManager == null) {
      return;
    }
    LynxIntersectionObserverManager manager = mManager.get();
    if (manager == null) {
      return;
    }
    RectF targetRect = manager.getBoundsOnScreenOfLynxBaseUI(target.ui);
    RectF intersectionRect =
        computeTargetAndRootIntersection((LynxBaseUI) target.ui, targetRect, rootRect);

    IntersectionObserverEntry entry = new IntersectionObserverEntry();
    if (intersectionRect != null && !intersectionRect.isEmpty()) {
      entry.isIntersecting = true;
    }
    entry.boundingClientRect = targetRect;
    entry.relativeRect = rootRect;
    entry.intersectionRect = intersectionRect;
    entry.time = 0; // TODO: time is not used currently
    entry.relativeToId =
        target.ui != null && target.ui.getIdSelector() != null ? target.ui.getIdSelector() : "";
    entry.update();

    IntersectionObserverEntry oldEntry = target.entry;
    target.entry = entry;

    final int jsCallbackId = target.jsCallbackId;

    boolean needNotifyJS = isInitial ? (mInitialRatio < entry.intersectionRatio)
                                     : hasCrossedThreshold(oldEntry, entry);
    if (needNotifyJS) {
      if (mIsCustomEventObserver) {
        manager.sendIntersectionObserverEvent(mAttachedUI.getSign(), entry.toDictionary());
      } else {
        // notify js
        manager.callIntersectionObserver(mObserverId, jsCallbackId, entry.toDictionary());
      }
    }
  }

  private boolean hasCrossedThreshold(
      IntersectionObserverEntry oldEntry, IntersectionObserverEntry newEntry) {
    float oldRatio =
        oldEntry != null && oldEntry.intersectionRect != null ? oldEntry.intersectionRatio : -1;
    float newRatio = newEntry.intersectionRect != null ? newEntry.intersectionRatio : -1;

    if (oldRatio == newRatio)
      return false;

    for (float threshold : mThresholds) {
      // Return true if an entry matches a threshold or if the new ratio
      // and the old ratio are on the opposite sides of a threshold.
      if (threshold == oldRatio || threshold == newRatio
          || (threshold < oldRatio != threshold < newRatio)) {
        return true;
      }
    }

    return false;
  }

  private RectF computeTargetAndRootIntersection(
      LynxBaseUI target, RectF targetRect, RectF rootRect) {
    if (!target.isVisible())
      return null;

    LynxIntersectionObserverManager manager = mManager.get();
    if (manager == null)
      return null;

    RectF intersectionRect = targetRect;
    LynxBaseUI parent = (LynxBaseUI) target.getParent();

    boolean atRoot = false;
    LynxBaseUI root = mRoot;
    if (root == null) {
      if (manager.getContext() == null || manager.getContext().getLynxUIOwner() == null) {
        LLog.e(TAG, "Get UI error");
      } else {
        root = manager.getContext().getLynxUIOwner().getRootUI();
      }
    }
    while (!atRoot && parent != null) {
      if (!parent.isVisible())
        return null;
      RectF parentRect = null;
      if (parent == root) {
        atRoot = true;
        if (mRelativeToScreen) {
          // parent is rootUi
          parentRect = manager.getBoundsOnScreenOfLynxBaseUI(parent);
        } else {
          parentRect = rootRect;
        }
      } else {
        // clip to bound
        if (parent.getOverflow() == 0) {
          parentRect = manager.getBoundsOnScreenOfLynxBaseUI(parent);
        }
      }

      if (parentRect != null) {
        if (parentRect.intersects(intersectionRect.left, intersectionRect.top,
                intersectionRect.right, intersectionRect.bottom)) {
          intersectionRect = new RectF(Math.max(parentRect.left, intersectionRect.left),
              Math.max(parentRect.top, intersectionRect.top),
              Math.min(parentRect.right, intersectionRect.right),
              Math.min(parentRect.bottom, intersectionRect.bottom));
        } else {
          intersectionRect = null;
        }
      }

      if (intersectionRect == null) {
        break;
      }

      parent = (LynxBaseUI) parent.getParent();
    }

    // in loop, screen's rect is not interacted
    if (mRelativeToScreen) {
      // before stepping in the function, it has already checked whether mManager is null.
      RectF boundsOfLynxView =
          manager.getBoundsOnScreenOfLynxBaseUI(manager.getContext().getLynxUIOwner().getRootUI());
      // from Lynx coordinate to screen coordinate
      targetRect.offset(boundsOfLynxView.left, boundsOfLynxView.top);
      if (intersectionRect != null) {
        intersectionRect.offset(boundsOfLynxView.left, boundsOfLynxView.top);
        if (!intersectionRect.intersect(rootRect)) {
          intersectionRect = null;
        }
      }
    }

    return intersectionRect;
  }

  private RectF getRootRect() {
    RectF rootRect = new RectF();
    LynxIntersectionObserverManager manager = mManager.get();
    if (manager == null) {
      return rootRect;
    }
    if (mRoot != null) {
      // relative to ui
      rootRect = manager.getBoundsOnScreenOfLynxBaseUI(mRoot);
    } else if (mRelativeToScreen) {
      // relative to screen
      rootRect = manager.getWindowRect(getContext());
    } else {
      // relative to LynxView
      rootRect = manager.getBoundsOnScreenOfLynxBaseUI(manager.getContext().getUIBody());
    }

    // extend rect with margins
    rootRect.left -= mMarginLeft;
    rootRect.right += mMarginRight;
    rootRect.top -= mMarginTop;
    rootRect.bottom += mMarginBottom;

    return rootRect;
  }
  private static class LynxUIObservationTarget {
    public LynxBaseUI ui;
    public int jsCallbackId;
    public IntersectionObserverEntry entry;
  }

  private static class IntersectionObserverEntry {
    public RectF relativeRect;
    public RectF boundingClientRect;
    public RectF intersectionRect;
    public float intersectionRatio;
    public boolean isIntersecting;
    public double time;
    public String relativeToId;

    public void update() {
      if (intersectionRect == null) {
        intersectionRatio = 0;
        return;
      }

      float targetArea = boundingClientRect.width() * boundingClientRect.height();
      float intersectionArea = intersectionRect.width() * intersectionRect.height();

      if (targetArea > 0) {
        intersectionRatio = intersectionArea / targetArea;
      } else {
        intersectionRatio = 0;
      }
    }

    public JavaOnlyMap toDictionary() {
      JavaOnlyMap data = new JavaOnlyMap();
      data.putMap("relativeRect", this.rectToDictionary(relativeRect));
      data.putMap("boundingClientRect", this.rectToDictionary(boundingClientRect));
      data.putMap("intersectionRect", this.rectToDictionary(intersectionRect));
      data.putDouble("intersectionRatio", intersectionRatio);
      data.putBoolean("isIntersecting", isIntersecting);
      data.putDouble("time", time);
      data.putString("observerId", relativeToId);
      return data;
    }

    private JavaOnlyMap rectToDictionary(RectF rect) {
      JavaOnlyMap data = new JavaOnlyMap();
      if (rect != null) {
        data.putDouble("left", rect.left);
        data.putDouble("right", rect.right);
        data.putDouble("top", rect.top);
        data.putDouble("bottom", rect.bottom);
      } else {
        data.putDouble("left", 0.f);
        data.putDouble("right", 0.f);
        data.putDouble("top", 0.f);
        data.putDouble("bottom", 0.f);
      }
      return data;
    }
  }
}
