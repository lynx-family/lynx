// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static com.lynx.jsbridge.LynxAccessibilityModule.MSG;
import static com.lynx.jsbridge.LynxAccessibilityModule.MSG_MUTATION_STYLES;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper.MUTATION_ACTION_STYLE_UPDATE;

import android.app.Service;
import android.graphics.Rect;
import android.os.Build;
import android.text.Layout;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeProvider;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxAccessibilityNodeProvider;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.text.FlattenUIText;
import com.lynx.tasm.behavior.ui.text.IUIText;
import com.lynx.tasm.behavior.ui.text.UIText;
import java.lang.ref.WeakReference;

/**
 * Lynx Accessibility Wrapper.
 * Provide unified Accessibility for Lynx SDK including all interface, a11y cloud tag and a11y
 * mutation ability.
 */
public class LynxAccessibilityWrapper implements LynxAccessibilityStateHelper.OnStateListener {
  private static final String TAG = "LynxAccessibilityWrapper";

  /**
   * The default accessibility status, which means status will consistent with
   * enableAccessibilityElement from page config
   */
  public static final int ACCESSIBILITY_ELEMENT_DEFAULT = -1;

  /** The accessibility status false, which means ui is not important for accessibility */
  public static final int ACCESSIBILITY_ELEMENT_FALSE = 0;

  /** The accessibility status true, which means ui is important for accessibility */
  public static final int ACCESSIBILITY_ELEMENT_TRUE = 1;

  /** Default bounds used to determine if the client didn't set any. */
  public static final Rect INVALID_BOUNDS =
      new Rect(Integer.MAX_VALUE, Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MIN_VALUE);

  /** Reference of host ui */
  private WeakReference<UIBody> mWeakHostUI = null;

  /** Page config enableNewAccessibility */
  private boolean mConfigEnableNewAccessibility = false;

  /** Page config enableA11y */
  private boolean mConfigEnableA11y = false;

  /** Page config enableAccessibilityElement */
  private boolean mConfigEnableAccessibilityElement = false;

  /** Page config enableOverlap */
  private boolean mConfigEnableOverlap = false;

  /** Page config enableA11yIDMutationObserver */
  private boolean mConfigEnableA11yIDMutationObserver = false;

  /** System state - whether accessibility is enabled */
  private boolean mAccessibilityEnable = false;

  /** System state - whether touch exploration is enabled */
  private boolean mTouchExplorationEnable = false;

  /** System accessibility manager */
  private AccessibilityManager mAccessibilityManager = null;

  /**
   * Accessibility state helper - unify logic about monitoring global accessibility and touch
   * exploration state.
   */
  private LynxAccessibilityStateHelper mStateHelper = null;

  /**
   * Accessibility mutation helper - unify the mutation relevant feature, including generating
   * mutation events and flushing events to FE.
   */
  private LynxAccessibilityMutationHelper mMutationHelper = null;

  /** The provider that exposes the virtual View tree to accessibility services. */
  private LynxAccessibilityNodeProvider mNodeProvider = null;

  /**
   * Implementing accessibility support in custom view that represent a collection of view-like
   * logical items.
   */
  private LynxAccessibilityDelegate mDelegate = null;

  private LynxAccessibilityHelper mHelper = null;

  public LynxAccessibilityWrapper(UIBody uiBody) {
    if (uiBody == null || uiBody.getBodyView() == null) {
      LLog.e(TAG, "Construct LynxAccessibilityWrapper with null host");
      return;
    }
    if (uiBody.getLynxContext() != null) {
      mAccessibilityManager = (AccessibilityManager) uiBody.getLynxContext().getSystemService(
          Service.ACCESSIBILITY_SERVICE);
    }
    mWeakHostUI = new WeakReference<>(uiBody);
    UIBody.UIBodyView bodyView = uiBody.getBodyView();
    LLog.i(TAG, "Construct LynxAccessibilityNodeProvider and set default delegate.");
    mNodeProvider = new LynxAccessibilityNodeProvider(uiBody);
    bodyView.setAccessibilityDelegate(new View.AccessibilityDelegate() {
      @Override
      public AccessibilityNodeProvider getAccessibilityNodeProvider(View host) {
        return mNodeProvider;
      }
    });
  }

  /** Unified page config parsing logic, and set switches to mProvider or mDelegate. */
  public void onPageConfigDecoded(final PageConfig config) {
    if (config == null) {
      return;
    }
    // The cloud tag plug-in relies on new accessibility impl which has some stability problems in
    // the lower app version. So we can provide a new switch to replace the old switch. However, the
    // old switch cannot be directly replaced, because many scenes do not have access to this
    // plug-in, so the old switch also needs to work.
    mConfigEnableNewAccessibility = config.getEnableNewAccessibility();
    mConfigEnableA11y = config.getEnableA11y();
    mConfigEnableAccessibilityElement = config.getEnableAccessibilityElement();
    mConfigEnableOverlap = config.getEnableOverlapForAccessibilityElement();
    mConfigEnableA11yIDMutationObserver = config.getEnableA11yIDMutationObserver();
    if (enableNodeProvider()) {
      mNodeProvider.setConfigEnableAccessibilityElement(mConfigEnableAccessibilityElement);
      mNodeProvider.setConfigEnableOverlapForAccessibilityElement(mConfigEnableOverlap);
    } else {
      UIBody uiBody = getUIBody();
      if (uiBody == null || uiBody.getBodyView() == null || mAccessibilityManager == null) {
        return;
      }
      if (mStateHelper == null) {
        mStateHelper = new LynxAccessibilityStateHelper(mAccessibilityManager, this);
      }
      if (mMutationHelper == null) {
        mMutationHelper = new LynxAccessibilityMutationHelper();
      }
      // Note: Here not use enableDelegate() or enableHelper() because mDelegate or mHelper is not
      // constructed and these methods always return false.
      if (mConfigEnableA11y) {
        initHelperIfNeeded(uiBody, uiBody.getBodyView());
      } else if (mConfigEnableNewAccessibility) {
        initDelegateIfNeeded(uiBody);
      }
    }
  }

  /**
   * Create and set LynxAccessibilityDelegate to the body view. LynxAccessibilityDelegate is used to
   * implement accessibility support in custom view that represent a collection of view-like logical
   * items.
   */
  private void initDelegateIfNeeded(@NonNull UIBody uiBody) {
    LLog.i(TAG,
        "init LynxAccessibilityDelegate with " + mConfigEnableNewAccessibility + ", "
            + mConfigEnableA11y);
    // create and register accessibility delegate to body view.
    if (mDelegate == null) {
      mDelegate = new LynxAccessibilityDelegate(uiBody);
    }
    mDelegate.setConfigEnableAccessibilityElement(mConfigEnableAccessibilityElement);
    ViewCompat.setAccessibilityDelegate(uiBody.getBodyView(), mDelegate);
  }

  /**
   * Create LynxAccessibilityHelper which means that we not create any flatten ui and always use
   * system View's accessibility api.
   */
  private void initHelperIfNeeded(@NonNull UIBody uiBody, @NonNull View bodyView) {
    LLog.i(TAG,
        "init LynxAccessibilityHelper with " + mConfigEnableNewAccessibility + ", "
            + mConfigEnableA11y);
    if (mHelper == null) {
      mHelper = new LynxAccessibilityHelper(uiBody);
    }
    mHelper.setConfigEnableAccessibilityElement(mConfigEnableAccessibilityElement);
    // Here we need to set IMPORTANT_FOR_ACCESSIBILITY_NO to avoid body view focused and set
    // AccessibilityDelegate to null because the delegate which has a LynxAccessibilityNodeProvider
    // is created when initializing body view.
    ViewCompat.setImportantForAccessibility(bodyView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
    ViewCompat.setAccessibilityDelegate(bodyView, null);
  }

  /**
   * Notified when an accessibility element focused.
   *
   * @param virtualID The identifier of the virtual view.
   * @param ui The ui that focused.
   */
  void onAccessibilityFocused(final int virtualID, final LynxBaseUI ui) {
    if (ui != null && ui.getLynxContext() != null) {
      JavaOnlyMap data = new JavaOnlyMap();
      data.put("element-id", ui.getSign());
      data.put("a11y-id", ui.getAccessibilityId());
      JavaOnlyArray params = new JavaOnlyArray();
      params.pushMap(data);
      ui.getLynxContext().sendGlobalEvent("activeElement", params);
    }
  }

  /**
   * Consume a hover event.
   *
   * @param event The motion event to be dispatched.
   * @return True if the event was handled by the view, false otherwise.
   */
  public boolean onHoverEvent(UIBody.UIBodyView bodyView, MotionEvent event) {
    if (bodyView != null
        && ((enableNodeProvider() && mNodeProvider.onHover(bodyView, event))
            || (enableDelegate() && mDelegate.dispatchHoverEvent(event)))) {
      return true;
    }
    return false;
  }

  /**
   * This method will be called when LynxView is destroyed to remove accessibility status listener
   * from AccessibilityManager.
   */
  public void onDestroy() {
    if (mStateHelper != null) {
      mStateHelper.removeAllListeners();
    }
  }

  /**
   * Mutation API - Insert accessibility mutation event.
   *
   * @param action The type of accessibility mutation event.
   * @param ui The ui which generate mutation event.
   */
  public void insertA11yMutationEvent(final int action, final LynxBaseUI ui) {
    if (mMutationHelper != null && shouldHandleA11yMutation()) {
      mMutationHelper.insertA11yMutationEvent(action, ui);
    }
  }

  /**
   * Mutation API - Insert accessibility style mutation event.
   *
   * @param ui The ui which generate style mutation event.
   * @param props The updated props.
   */
  public void handleMutationStyleUpdate(final LynxBaseUI ui, final StylesDiffMap props) {
    if (mMutationHelper != null && shouldHandleA11yMutation() && ui != null && props != null) {
      ReadableMap propMap = props.mBackingMap;
      ReadableMapKeySetIterator iterator = propMap.keySetIterator();
      while (iterator.hasNextKey()) {
        String key = iterator.nextKey();
        mMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_STYLE_UPDATE, ui, key);
      }
    }
  }

  /** Mutation API - Flush all mutation events to FE. */
  public void flushA11yMutationEvents() {
    if (mMutationHelper != null && shouldHandleA11yMutation() && getUIBody() != null
        && getUIBody().getLynxContext() != null) {
      mMutationHelper.flushA11yMutationEvents(getUIBody().getLynxContext());
    }
  }

  /** Mutation API - Register style whose mutations will be concerned. */
  public void registerMutationStyleInner(final ReadableMap params, final JavaOnlyMap res) {
    if (mMutationHelper == null || (!enableDelegate() && !enableHelper())) {
      res.putString(MSG, "Fail: init accessibility mutation env error");
      return;
    }
    ReadableArray paramsArray = params.getArray(MSG_MUTATION_STYLES, null);
    if (paramsArray == null) {
      res.putString(MSG, "Fail: params error with key" + MSG_MUTATION_STYLES);
      return;
    }
    mMutationHelper.registerMutationStyle(paramsArray);
    res.putString(MSG, "Success: finish register");
  }

  /**  Cloud tag API - Attempts to give accessibility focus to a virtual view. */
  public void requestAccessibilityFocus(final LynxBaseUI ui, final Callback callback) {
    if (!isSystemAccessibilityEnable()) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "System accessibility is disable!");
    } else if (ui == null) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "Focus node is null!");
    } else if ((enableHelper() && mHelper.requestAccessibilityFocus(ui))
        || (enableDelegate() && mDelegate.requestAccessibilityFocus(ui))) {
      callback.invoke(LynxUIMethodConstants.SUCCESS, "Accessibility element on focused");
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "Request accessibility focus fail!");
    }
  }

  /** Cloud tag API - Fetch all a11y element of given ui. */
  public void fetchAccessibilityTargets(final LynxBaseUI ui, final Callback callback) {
    if (ui != null) {
      if (enableDelegate() || enableHelper()) {
        final JavaOnlyArray res = new JavaOnlyArray();
        fetchAccessibilityTargetsInternal(ui, false, res);
        callback.invoke(LynxUIMethodConstants.SUCCESS, res);
      } else {
        callback.invoke(LynxUIMethodConstants.UNKNOWN, "fetch accessibility targets fail!");
      }
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "No accessibility element found!");
    }
  }

  /** Cloud tag API - Fetch all text content of given ui. */
  public void innerText(final LynxBaseUI ui, final Callback callback) {
    if (ui != null) {
      if (enableDelegate() || enableHelper()) {
        final JavaOnlyArray res = new JavaOnlyArray();
        fetchAccessibilityTargetsInternal(ui, true, res);
        callback.invoke(LynxUIMethodConstants.SUCCESS, res);
      } else {
        callback.invoke(LynxUIMethodConstants.UNKNOWN, "fetch accessibility inner text fail!");
      }
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "No accessibility element found!");
    }
  }

  /**
   * Attempts to fetch all accessibility elements or inner text of target ui.
   *
   * @param target the target ui
   * @param fetchText whether return inner text
   * @param res fetch result
   */
  private void fetchAccessibilityTargetsInternal(
      final LynxBaseUI target, boolean fetchText, final JavaOnlyArray res) {
    if (target != null && res != null) {
      if (!fetchText) {
        JavaOnlyMap info = new JavaOnlyMap();
        info.putInt("element-id", target.getSign());
        info.putString("a11y-id", target.getAccessibilityId());
        res.add(info);
      } else if (target instanceof UIText) {
        CharSequence text = ((UIText) target).getOriginText();
        res.pushString(text == null ? "" : text.toString());
      } else if (target instanceof FlattenUIText) {
        CharSequence text = ((FlattenUIText) target).getOriginText();
        res.pushString(text == null ? "" : text.toString());
      }
      for (int i = 0; i < target.getChildren().size(); ++i) {
        fetchAccessibilityTargetsInternal(target.getChildren().get(i), fetchText, res);
      }
    }
  }

  public boolean shouldHandleA11yMutation() {
    return mConfigEnableA11yIDMutationObserver && isSystemAccessibilityEnable()
        && (enableDelegate() || enableHelper());
  }

  /**
   * Getting system accessibility's status.
   *
   * @return Whether the system accessibility is enabled.
   */
  public boolean isSystemAccessibilityEnable() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      // TouchExplorationStateChangeListener is added in android 4.4
      return mAccessibilityManager != null && mAccessibilityEnable && mTouchExplorationEnable;
    } else {
      return mAccessibilityManager != null && mAccessibilityEnable
          && mAccessibilityManager.isTouchExplorationEnabled();
    }
  }

  public void onLayoutFinish() {
    flushA11yMutationEvents();
    if (enableHelper()) {
      mHelper.applyAccessibilityElements();
    }
  }

  public void addAccessibilityElementsUI(final LynxBaseUI root) {
    if (root != null && enableHelper()) {
      mHelper.addAccessibilityElementsUI(root);
    }
  }

  public void addAccessibilityElementsA11yUI(final LynxBaseUI root) {
    if (root != null && enableHelper()) {
      mHelper.addAccessibilityElementsA11yUI(root);
    }
  }

  public void addOrRemoveUIFromExclusiveMap(LynxBaseUI ui, boolean isExclusive) {
    if (!enableHelper()) {
      return;
    }
    if (isExclusive) {
      mHelper.addUIToExclusiveMap(ui);
    } else {
      mHelper.removeUIFromExclusiveMap(ui);
    }
  }

  public boolean shouldCreateNoFlattenUI() {
    return enableHelper() && isSystemAccessibilityEnable();
  }

  @Override
  public void onAccessibilityEnable(boolean enable) {
    mAccessibilityEnable = enable;
  }

  @Override
  public void onTouchExplorationEnable(boolean enable) {
    mTouchExplorationEnable = enable;
  }

  private boolean enableNodeProvider() {
    return !mConfigEnableNewAccessibility && !mConfigEnableA11y && mNodeProvider != null;
  }

  public boolean enableDelegate() {
    return mConfigEnableNewAccessibility && !mConfigEnableA11y && mDelegate != null;
  }

  public boolean enableHelper() {
    return !mConfigEnableNewAccessibility && mConfigEnableA11y && mHelper != null;
  }

  public LynxAccessibilityHelper getLynxAccessibilityHelper() {
    return mHelper;
  }

  private UIBody getUIBody() {
    if (mWeakHostUI != null) {
      return mWeakHostUI.get();
    }
    return null;
  }
}
