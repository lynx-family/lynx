// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.UiThread;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxHostEnvironment;
import com.lynx.tasm.LynxPageView;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.BuiltInUIRegistry;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.WeakHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Owns the shared configuration and lifetime of independent surface pages.
 * Obtain a host through {@link LynxViewGroupBuilder#buildHost()}.
 * Configure modules and behaviors on the builder before creating the host.
 */
public final class LynxHost {
  private static final AtomicLong sNextHostId = new AtomicLong();

  private final LynxHostEnvironment mEnvironment;
  private final int mEmbeddedMode;
  private final String mSharedJSContextGroupId;
  // Accessed on the UI thread; abandoned surfaces are not retained by the host.
  private final WeakHashMap<LynxPageView, Boolean> mSurfaces = new WeakHashMap<>();
  private final AtomicBoolean mReleased = new AtomicBoolean();

  LynxHost(LynxViewGroupBuilder builder) {
    mEmbeddedMode = builder.embeddedMode;
    BehaviorRegistry registry = builder.behaviorRegistry;
    BehaviorRegistry behaviors = new BehaviorRegistry();
    for (String name : registry.getAllBehaviorRegistryName()) {
      behaviors.addBehavior(registry.get(name));
    }
    if (EmbeddedMode.isBaseModeEnable(mEmbeddedMode)) {
      behaviors.setBuiltInBehaviors(BuiltInUIRegistry.getInstance().getBuiltInUIBehaviors());
    }
    mEnvironment = new LynxHostEnvironment(builder.lynxRuntimeOptions, behaviors);
    LynxGroup group = mEnvironment.getRuntimeOptions().getLynxGroup();
    String groupId = group == null ? null : group.getID();
    mSharedJSContextGroupId = groupId != null && !LynxGroup.SINGNLE_GROUP.equals(groupId)
        ? groupId
        : "lynx-surface-" + sNextHostId.getAndIncrement();
  }

  /**
   * Creates a page using the supplied Android context, with its own native shell.
   * Pages share a JS context, including when the
   * configured LynxGroup requests isolated contexts. The supplied LynxGroup is not modified.
   * No template is loaded; use the surface's loadTemplate method after creation.
   */
  @UiThread
  @NonNull
  public LynxPageView createPageView(@NonNull Context context) {
    UIThreadUtils.assertOnUiThread();
    if (mReleased.get()) {
      throw new IllegalStateException("Cannot create a surface from a released host");
    }
    LynxPageView surface;
    TraceEvent.beginSection("LynxPageView.create");
    try {
      surface = new LynxPageView(context);
    } finally {
      TraceEvent.endSection("LynxPageView.create");
    }
    if (mReleased.get()) {
      surface.destroy();
      throw new IllegalStateException("Cannot create a surface from a released host");
    }
    mSurfaces.put(surface, true);
    return surface;
  }

  /** Returns the embedded rendering policy shared by this host's pages. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public int getEmbeddedMode() {
    return mEmbeddedMode;
  }

  /** Returns the environment shared by this host's surfaces. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public LynxHostEnvironment getHostEnvironment() {
    return mEnvironment;
  }

  /** Closes all live surfaces on the UI thread. Repeated calls have no effect. */
  public void release() {
    if (!mReleased.compareAndSet(false, true)) {
      return;
    }
    UIThreadUtils.runOnUiThreadImmediately(this::releaseOnUiThread);
  }

  private void releaseOnUiThread() {
    RuntimeException failure = null;
    for (LynxPageView surface : new ArrayList<>(mSurfaces.keySet())) {
      try {
        surface.destroy();
      } catch (RuntimeException error) {
        if (failure == null) {
          failure = error;
        } else if (failure != error) {
          failure.addSuppressed(error);
        }
      }
    }
    mSurfaces.clear();
    if (failure != null) {
      throw failure;
    }
  }
}
