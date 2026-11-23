// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;

/** Thin engine-side bridge for the optional Host Script View hooks. */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxHostScriptViewHooks {
  private static volatile Delegate sDelegate;

  private LynxHostScriptViewHooks() {}

  /** Implementation supplied by the optional Host Script library on install. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public interface Delegate {
    boolean isEnabled();

    void registerView(@NonNull LynxView view, long viewId);

    void unregisterView(long viewId);

    void clearViewUrl(long viewId);

    @Nullable
    LynxViewBuilder beforeCreate(@Nullable Context context, @Nullable LynxViewBuilder builder);

    @NonNull LynxLoadMeta beforeLoadTemplate(long viewId, @NonNull LynxLoadMeta meta);

    @NonNull LynxUpdateMeta beforeUpdateMetaData(long viewId, @NonNull LynxUpdateMeta meta);

    @Nullable TemplateData beforeUpdateGlobalProps(long viewId, @Nullable TemplateData props);

    long beforeUpdateViewport(long viewId, int widthMeasureSpec, int heightMeasureSpec);
  }

  public static void setDelegate(@Nullable Delegate delegate) {
    sDelegate = delegate;
  }

  @Nullable
  public static Delegate getDelegate() {
    return sDelegate;
  }

  static boolean isEnabled() {
    Delegate delegate = sDelegate;
    return delegate != null && delegate.isEnabled();
  }

  static void register(LynxView view, @Nullable LynxTemplateRender render) {
    Delegate delegate = sDelegate;
    if (delegate == null) {
      return;
    }
    long viewId = viewId(render);
    if (viewId != LynxLogContext.UNAVAILABLE_ID) {
      delegate.registerView(view, viewId);
    }
  }

  static void unregister(@Nullable LynxTemplateRender render) {
    Delegate delegate = sDelegate;
    if (delegate == null) {
      return;
    }
    long viewId = viewId(render);
    if (viewId != LynxLogContext.UNAVAILABLE_ID) {
      delegate.unregisterView(viewId);
    }
  }

  static void clearViewUrl(long viewId) {
    Delegate delegate = sDelegate;
    if (delegate != null) {
      delegate.clearViewUrl(viewId);
    }
  }

  static LynxLoadMeta beforeLoadTemplate(@Nullable LynxTemplateRender render, LynxLoadMeta meta) {
    Delegate delegate = sDelegate;
    return delegate == null ? meta : delegate.beforeLoadTemplate(viewId(render), meta);
  }

  static LynxUpdateMeta beforeUpdateMetaData(
      @Nullable LynxTemplateRender render, LynxUpdateMeta meta) {
    Delegate delegate = sDelegate;
    return delegate == null ? meta : delegate.beforeUpdateMetaData(viewId(render), meta);
  }

  @Nullable
  static TemplateData beforeUpdateGlobalProps(
      @Nullable LynxTemplateRender render, @Nullable TemplateData props) {
    Delegate delegate = sDelegate;
    return delegate == null ? props : delegate.beforeUpdateGlobalProps(viewId(render), props);
  }

  @Nullable
  static LynxViewBuilder beforeCreate(
      @Nullable Context context, @Nullable LynxViewBuilder builder) {
    Delegate delegate = sDelegate;
    return delegate == null ? builder : delegate.beforeCreate(context, builder);
  }

  /** Returns packed (widthMeasureSpec << 32) | (heightMeasureSpec & 0xffffffffL). */
  static long beforeUpdateViewport(
      @Nullable LynxTemplateRender render, int widthMeasureSpec, int heightMeasureSpec) {
    Delegate delegate = sDelegate;
    if (delegate == null) {
      return ((long) widthMeasureSpec << 32) | (heightMeasureSpec & 0xffffffffL);
    }
    return delegate.beforeUpdateViewport(viewId(render), widthMeasureSpec, heightMeasureSpec);
  }

  private static long viewId(@Nullable LynxTemplateRender render) {
    return render == null ? LynxLogContext.UNAVAILABLE_ID : render.getLogContextSnapshot().viewId;
  }
}
