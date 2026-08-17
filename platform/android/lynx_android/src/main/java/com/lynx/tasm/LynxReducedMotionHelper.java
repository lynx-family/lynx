// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.content.ContentResolver;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import java.util.ArrayList;
import java.util.List;
import java.util.WeakHashMap;

@RestrictTo(RestrictTo.Scope.LIBRARY)
final class LynxReducedMotionHelper {
  private static volatile LynxReducedMotionHelper sInstance;

  private final ContentResolver mContentResolver;
  private final ContentObserver mObserver;
  private final WeakHashMap<LynxTemplateRender, Boolean> mTemplateRenders = new WeakHashMap<>();
  private boolean mStarted;
  private volatile boolean mReducedMotionEnabled;

  static LynxReducedMotionHelper getInstance(@NonNull ContentResolver contentResolver) {
    if (sInstance == null) {
      synchronized (LynxReducedMotionHelper.class) {
        if (sInstance == null) {
          sInstance = new LynxReducedMotionHelper(contentResolver);
        }
      }
    }
    return sInstance;
  }

  LynxReducedMotionHelper(@NonNull ContentResolver contentResolver) {
    mContentResolver = contentResolver;
    mObserver = new ContentObserver(new Handler(Looper.getMainLooper())) {
      @Override
      public void onChange(boolean selfChange) {
        notifyReducedMotionChanged();
      }
    };
  }

  synchronized void start(@NonNull LynxTemplateRender templateRender) {
    mTemplateRenders.put(templateRender, true);
    if (mStarted) {
      return;
    }
    mReducedMotionEnabled = readReducedMotionEnabled();
    try {
      mContentResolver.registerContentObserver(getAnimatorDurationScaleUri(), false, mObserver);
      mContentResolver.registerContentObserver(getTransitionAnimationScaleUri(), false, mObserver);
      mStarted = true;
    } catch (SecurityException ignored) {
      try {
        mContentResolver.unregisterContentObserver(mObserver);
      } catch (IllegalArgumentException | SecurityException unregisterException) {
      }
      mStarted = false;
    }
  }

  synchronized void stop(@NonNull LynxTemplateRender templateRender) {
    mTemplateRenders.remove(templateRender);
    if (!mStarted || !mTemplateRenders.isEmpty()) {
      return;
    }
    try {
      mContentResolver.unregisterContentObserver(mObserver);
    } catch (IllegalArgumentException | SecurityException ignored) {
    }
    mStarted = false;
  }

  private void notifyReducedMotionChanged() {
    List<LynxTemplateRender> templateRenders;
    synchronized (this) {
      templateRenders = new ArrayList<>(mTemplateRenders.keySet());
    }
    boolean reducedMotion = readReducedMotionEnabled();
    mReducedMotionEnabled = reducedMotion;
    for (LynxTemplateRender templateRender : templateRenders) {
      if (templateRender != null) {
        templateRender.updateReducedMotion(reducedMotion);
      }
    }
  }

  boolean isReducedMotionEnabled() {
    return mReducedMotionEnabled;
  }

  private boolean readReducedMotionEnabled() {
    try {
      float animatorScale;
      float transitionScale;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
        animatorScale = Settings.Global.getFloat(
            mContentResolver, Settings.Global.ANIMATOR_DURATION_SCALE, 1.0f);
        transitionScale = Settings.Global.getFloat(
            mContentResolver, Settings.Global.TRANSITION_ANIMATION_SCALE, 1.0f);
      } else {
        animatorScale = Settings.System.getFloat(
            mContentResolver, Settings.System.ANIMATOR_DURATION_SCALE, 1.0f);
        transitionScale = Settings.System.getFloat(
            mContentResolver, Settings.System.TRANSITION_ANIMATION_SCALE, 1.0f);
      }
      return isReducedMotionEnabled(animatorScale, transitionScale);
    } catch (SecurityException ignored) {
      return false;
    }
  }

  static boolean isReducedMotionEnabled(float animatorScale, float transitionScale) {
    return animatorScale == 0.0f || transitionScale == 0.0f;
  }

  private static Uri getAnimatorDurationScaleUri() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      return Settings.Global.getUriFor(Settings.Global.ANIMATOR_DURATION_SCALE);
    }
    return Settings.System.getUriFor(Settings.System.ANIMATOR_DURATION_SCALE);
  }

  private static Uri getTransitionAnimationScaleUri() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      return Settings.Global.getUriFor(Settings.Global.TRANSITION_ANIMATION_SCALE);
    }
    return Settings.System.getUriFor(Settings.System.TRANSITION_ANIMATION_SCALE);
  }
}
