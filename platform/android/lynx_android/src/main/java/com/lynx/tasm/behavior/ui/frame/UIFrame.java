// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import android.content.Context;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxFrameViewProvider;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

@RestrictTo(RestrictTo.Scope.LIBRARY)
@LynxGeneratorName(packageName = "com.lynx.tasm.behavior.ui.frame")
@LynxBehavior(tagName = "frame", isCreateAsync = false)
public final class UIFrame extends LynxUI<LynxFrameView> {
  private static final String TAG = "UIFrame";
  private static final String PROP_DATA = "data";
  private static final String PROP_GLOBAL_PROPS = "global-props";
  private static final String DETAIL_KEY_FRAME_URL = "frameUrl";
  private static final String DETAIL_KEY_INTRINSIC_WIDTH = "intrinsicWidth";
  private static final String DETAIL_KEY_INTRINSIC_HEIGHT = "intrinsicHeight";
  private TemplateBundle mPendingBundle;
  private boolean mIsPropsUpdated = false;
  private boolean mIsUrlChanged = false;
  private boolean mHasIntrinsicContentSize = false;
  private double mIntrinsicWidth = 0;
  private double mIntrinsicHeight = 0;

  private static final class FrameEventCallback implements LynxFrameView.FrameEventCallback {
    private final WeakReference<UIFrame> mFrameUIRef;

    FrameEventCallback(UIFrame frameUI) {
      mFrameUIRef = new WeakReference<>(frameUI);
    }

    @Override
    public void onIntrinsicContentSizeChanged(int width, int height) {
      UIFrame frameUI = mFrameUIRef.get();
      if (frameUI != null) {
        frameUI.onIntrinsicContentSizeChanged(width, height);
      }
    }
  }

  private int convertPresetLengthToPx(String value) {
    if (value == null) {
      return -1;
    }
    UIBody uiBody = getLynxContext().getUIBody();
    float rootFontSize = 0;
    float rootWidth = 0;
    float rootHeight = 0;
    if (uiBody != null) {
      rootFontSize = uiBody.getFontSize();
      rootWidth = uiBody.getWidth();
      rootHeight = uiBody.getHeight();
    }
    return (int) UnitUtils.toPxWithDisplayMetrics(value, rootFontSize, getFontSize(), rootWidth,
        rootHeight, MeasureUtils.UNDEFINED, getLynxContext().getScreenMetrics());
  }

  public UIFrame(LynxContext context) {
    this(context, null);
  }

  public UIFrame(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected LynxFrameView createView(Context context) {
    LynxFrameView frameView = null;
    LynxContext lynxContext = (LynxContext) mContext;
    LynxFrameViewProvider provider = lynxContext.getLynxFrameViewProvider();
    if (provider != null) {
      frameView = provider.getLynxFrameView(lynxContext);
    }

    if (frameView == null) {
      frameView = new LynxFrameView(mContext);
    }
    frameView.init(lynxContext);
    frameView.setFrameEventCallback(new FrameEventCallback(this));
    return frameView;
  }

  @Override
  public void setSign(int sign, String tagName) {
    super.setSign(sign, tagName);
    LynxFrameView view = getView();
    if (view != null) {
      view.setSign(sign);
    }
  }

  @Override
  public void updateExtraData(Object data) {
    if (data instanceof TemplateBundle) {
      TemplateBundle bundle = (TemplateBundle) data;
      if (mIsUrlChanged && mIsPropsUpdated && loadBundle(bundle)) {
        mPendingBundle = null;
        mIsUrlChanged = false;
        mIsPropsUpdated = false;
      } else {
        mPendingBundle = bundle;
      }
    }
  }

  private boolean loadBundle(TemplateBundle bundle) {
    LynxFrameView view = getView();
    if (view == null) {
      return false;
    }
    // need to establish the parent-child UI relationship before loadBundle currently
    // TODO(hexionghui): fix it later
    attachPageUICallback();
    return view.loadBundle(bundle);
  }

  private void attachPageUICallback() {
    LynxFrameView view = getView();
    if (view == null) {
      return;
    }
    // TODO(hexionghui): remove callback on uibody
    view.setAttachLynxPageUICallback(new UIBody.UIBodyView.attachLynxPageUICallback() {
      final private WeakReference<LynxBaseUI> mPageUI = new WeakReference<>(UIFrame.this);
      @Override
      public void attachLynxPageUI(@NonNull WeakReference<Object> ui) {
        if (!(ui.get() instanceof UIBody)) {
          return;
        }
        UIBody rootUI = (UIBody) ui.get();
        if (rootUI.getLynxContext() == null) {
          return;
        }
        rootUI.getLynxContext().EnsureEventDispatcher();
        LynxBaseUI pageUI = mPageUI.get();
        if (pageUI == null) {
          return;
        }
        if (pageUI.getChildrenLynxPageUI() == null) {
          pageUI.setChildrenLynxPageUI(new HashMap<>());
        }
        pageUI.getChildrenLynxPageUI().put(String.valueOf(System.identityHashCode(pageUI)), rootUI);
        if (pageUI.getLynxContext() != null && pageUI.getLynxContext().getLynxUIOwner() != null
            && pageUI.getLynxContext().getLynxUIOwner().getRootUI() != null) {
          rootUI.setParentLynxPageUI(pageUI.getLynxContext().getLynxUIOwner().getRootUI());
        }
        if (rootUI.getView() != null) {
          rootUI.getView().setIsChildLynxPageUI(true);
        }
      }
    });
  }

  @Override
  public void destroy() {
    super.destroy();
    LynxFrameView view = getView();
    if (view != null) {
      view.setFrameEventCallback(null);
      view.destroy();
    }
  }

  @Override
  public void updateLayout(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bound) {
    super.updateLayout(left, top, width, height, paddingLeft, paddingTop, paddingRight,
        paddingBottom, marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth,
        borderTopWidth, borderRightWidth, borderBottomWidth, bound);

    LynxFrameView view = getView();
    if (view == null) {
      return;
    }
    int horizontalInsets = paddingLeft + paddingRight + borderLeftWidth + borderRightWidth;
    int verticalInsets = paddingTop + paddingBottom + borderTopWidth + borderBottomWidth;
    int contentWidth = Math.max(0, width - horizontalInsets);
    int contentHeight = Math.max(0, height - verticalInsets);
    view.updateLayout(contentWidth, contentHeight);
  }

  @Override
  protected Map<String, Object> buildLayoutChangeEventDetail() {
    Map<String, Object> detail = super.buildLayoutChangeEventDetail();
    LynxFrameView view = getView();
    detail.put(DETAIL_KEY_FRAME_URL, view == null ? "" : view.getUrl());
    if (mHasIntrinsicContentSize) {
      detail.put(DETAIL_KEY_INTRINSIC_WIDTH, mIntrinsicWidth);
      detail.put(DETAIL_KEY_INTRINSIC_HEIGHT, mIntrinsicHeight);
    }
    return detail;
  }

  void onIntrinsicContentSizeChanged(int width, int height) {
    DisplayMetrics screenMetrics = getLynxContext().getScreenMetrics();
    float density = screenMetrics == null ? 0 : screenMetrics.density;
    mIntrinsicWidth = density > 0 ? width / (double) density : width;
    mIntrinsicHeight = density > 0 ? height / (double) density : height;
    mHasIntrinsicContentSize = true;
    sendLayoutChangeEvent();
  }

  private void resetIntrinsicContentSize() {
    mHasIntrinsicContentSize = false;
    mIntrinsicWidth = 0;
    mIntrinsicHeight = 0;
  }

  @LynxProp(name = PROP_DATA)
  public void setData(long value) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setInitData(value);
    }
    mProps.remove(PROP_DATA);
  }

  @LynxProp(name = "src")
  public void setSrc(String value) {
    mIsUrlChanged = true;
    mIsPropsUpdated = false;
    resetIntrinsicContentSize();
    LynxFrameView view = getView();
    if (view == null) {
      return;
    }
    view.setUrl(value);
  }

  @LynxProp(name = PROP_GLOBAL_PROPS)
  public void setGlobalProps(long value) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setGlobalProps(value);
    }
    mProps.remove(PROP_GLOBAL_PROPS);
  }

  @LynxProp(name = "embedded-mode")
  public void setEmbeddedMode(@EmbeddedMode.Mode int mode) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setEmbeddedMode(mode);
    }
  }

  @LynxProp(name = "auto-width", defaultBoolean = false)
  public void setAutoWidth(boolean value) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setAutoWidth(value);
    }
  }

  @LynxProp(name = "auto-height", defaultBoolean = false)
  public void setAutoHeight(boolean value) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setAutoHeight(value);
    }
  }

  @LynxProp(name = "preset-width")
  public void setPresetWidth(String width) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setPresetWidth(convertPresetLengthToPx(width));
    }
  }

  @LynxProp(name = "preset-height")
  public void setPresetHeight(String height) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setPresetHeight(convertPresetLengthToPx(height));
    }
  }

  @LynxProp(name = "enable-multi-async-thread")
  public void setEnableMultiAsyncThread(Boolean value) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setEnableMultiAsyncThread(value);
    }
  }

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    if (mIsUrlChanged) {
      if (mPendingBundle != null && loadBundle(mPendingBundle)) {
        mPendingBundle = null;
        mIsUrlChanged = false;
      }
    }
    mIsPropsUpdated = true;

    LynxFrameView view = getView();
    if (view != null) {
      view.onPropsUpdated();
    }
  }
}
