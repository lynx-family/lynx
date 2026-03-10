// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import android.content.Context;
import android.graphics.Rect;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.LynxUpdateMeta;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import java.lang.ref.WeakReference;
import java.util.HashMap;

@RestrictTo(RestrictTo.Scope.LIBRARY)
@LynxGeneratorName(packageName = "com.lynx.tasm.behavior.ui.frame")
@LynxBehavior(tagName = "frame", isCreateAsync = true)
public final class UIFrame extends LynxUI<LynxFrameView> {
  private static final String TAG = "UIFrame";
  private TemplateBundle mPendingBundle;
  private boolean mIsPropsUpdated = false;
  private boolean mIsUrlChanged = false;

  public UIFrame(LynxContext context) {
    this(context, null);
  }

  public UIFrame(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected LynxFrameView createView(Context context) {
    return new LynxFrameView(mContext);
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

  // TODO(zhoupeng): do not convet LepusValue to ReadableMap
  @LynxProp(name = "data")
  public void setData(ReadableMap value) {
    if (!(value instanceof JavaOnlyMap)) {
      LLog.e(TAG, "prop data is not a JavaOnlyMap");
      return;
    }
    LynxFrameView view = getView();
    if (view != null) {
      view.setInitData(TemplateData.fromMap((JavaOnlyMap) value));
    }
  }

  @LynxProp(name = "src")
  public void setSrc(String value) {
    mIsUrlChanged = true;
    mIsPropsUpdated = false;
    LynxFrameView view = getView();
    if (view == null) {
      return;
    }
    view.setUrl(value);
  }

  @LynxProp(name = "global-props")
  public void setGlobalProps(ReadableMap value) {
    if (!(value instanceof JavaOnlyMap)) {
      LLog.e(TAG, "global props data is not a JavaOnlyMap");
      return;
    }
    LynxFrameView view = getView();
    if (view != null) {
      view.setGlobalProps(TemplateData.fromMap((JavaOnlyMap) value));
    }
  }

  @LynxProp(name = "embedded-mode")
  public void setEmbeddedMode(@EmbeddedMode.Mode int mode) {
    LynxFrameView view = getView();
    if (view != null) {
      view.setEmbeddedMode(mode);
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
