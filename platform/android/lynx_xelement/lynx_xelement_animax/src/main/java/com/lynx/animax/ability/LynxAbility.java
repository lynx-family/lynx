// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.ability;

import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.animax.base.AnimaXError;
import com.lynx.animax.listener.AnimaXErrorParam;
import com.lynx.animax.listener.AnimaXFPSParam;
import com.lynx.animax.listener.AnimaXParam;
import com.lynx.animax.listener.AnimaXTapParam;
import com.lynx.animax.listener.IAnimationListener;
import com.lynx.animax.service.ServiceScope;
import com.lynx.animax.util.LynxAnimaX;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.image.ImageUrlRedirectUtils;
import com.lynx.tasm.event.LynxCustomEvent;
import java.lang.ref.WeakReference;

/**
 * Default ability implementation for Lynx "animax-view"
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class LynxAbility extends BaseAbility implements IAnimationListener {
  private static final String TAG = "LynxAbility";

  private static final String EVENT_COMPLETION = "completion";
  private static final String EVENT_START = "start";
  private static final String EVENT_REPEAT = "repeat";
  private static final String EVENT_CANCEL = "cancel";
  private static final String EVENT_READY = "ready";
  private static final String EVENT_UPDATE = "update";
  private static final String EVENT_ERROR = "error";
  private static final String EVENT_FPS = "fps";
  private static final String EVENT_TAP_LAYERS = "taplayers";
  private static final String EVENT_FIRST_FRAME = "firstframe";
  private static final String EVENT_COMPOSITION_READY = "compositionready";
  private static final String EVENT_WARNING = "warning";

  private boolean mListenUpdate = true;

  @NonNull private final WeakReference<LynxBaseUI> mUI;
  @NonNull private final WeakReference<LynxContext> mContext;

  public LynxAbility(LynxBaseUI ui, LynxContext context) {
    mUI = new WeakReference<>(ui);
    mContext = new WeakReference<>(context);
    addAnimationListener(this);
  }

  public void setListenUpdate(boolean update) {
    mListenUpdate = update;
  }

  public boolean getListenUpdate() {
    return mListenUpdate;
  }

  @Override
  public void onStart(AnimaXParam event) {
    sendEventToJs(EVENT_START, event);
  }

  @Override
  public void onReady(AnimaXParam event) {
    sendEventToJs(EVENT_READY, event);
  }

  @Override
  public void onComplete(AnimaXParam event) {
    sendEventToJs(EVENT_COMPLETION, event);
  }

  @Override
  public void onRepeat(AnimaXParam event) {
    sendEventToJs(EVENT_REPEAT, event);
  }

  @Override
  public void onCancel(AnimaXParam event) {
    sendEventToJs(EVENT_CANCEL, event);
  }

  @Override
  public void onWarning(AnimaXErrorParam event) {
    sendEventToJs(EVENT_WARNING, event);
    showLynxLogBox(event, LynxError.LEVEL_WARN);
  }

  @Override
  public void onError(AnimaXErrorParam event) {
    int errorCode = event.getErrorCode();
    if (errorCode == AnimaXError.VIDEO_PLAYER_ERROR.getErrorCode()
        || errorCode == AnimaXError.VIDEO_PLAYER_ERROR_HAS_OCCURRED.getErrorCode()) {
      // TODO(aiyongbiao.rick): Skip alpha video decode error, need only report this error in the
      // player side.
      return;
    }
    sendEventToJs(EVENT_ERROR, event);
    showLynxLogBox(event, LynxError.LEVEL_ERROR);
  }

  @Override
  public void onUpdate(AnimaXParam event) {
    if (mListenUpdate) {
      sendEventToJs(EVENT_UPDATE, event);
    }
  }

  @Override
  public void onFPS(AnimaXFPSParam event) {
    sendEventToJs(EVENT_FPS, event);
  }

  @Override
  public void onTapLayers(AnimaXTapParam event) {
    sendEventToJs(EVENT_TAP_LAYERS, event);
  }

  @Override
  public void onFirstFrame(AnimaXParam event) {
    sendEventToJs(EVENT_FIRST_FRAME, event);
  }

  @Override
  public void onCompositionReady(AnimaXParam event) {
    sendEventToJs(EVENT_COMPOSITION_READY, event);
  }

  private void sendEventToJs(String eventName, AnimaXParam event) {
    LynxBaseUI ui = mUI.get();
    LynxContext context = mContext.get();
    if (ui == null || context == null) {
      return;
    }

    EventEmitter emitter = context.getEventEmitter();
    if (emitter == null) {
      return;
    }

    emitter.sendCustomEvent(new LynxCustomEvent(ui.getSign(), eventName, event.getOriginParams()) {
      @Override
      public String paramsName() {
        return "detail";
      }
    });
  }

  private void showLynxLogBox(AnimaXErrorParam errorInfo, String level) {
    LynxContext context = mContext.get();
    if (context == null) {
      return;
    }
    LynxError error = new LynxError(
        errorInfo.getErrorCode(), errorInfo.getErrorMessage(), "", level, LynxError.JS_ERROR);
    error.setLogBoxOnly(true);
    context.handleLynxError(error);
  }

  @Override
  public String redirectUrl(String originUrl) {
    LynxContext context = mContext.get();
    if (context == null) {
      return originUrl;
    }

    return ImageUrlRedirectUtils.redirectUrl(context, originUrl);
  }

  @Override
  protected ServiceScope getScope() {
    return LynxAnimaX.inst().getScope();
  }
}
