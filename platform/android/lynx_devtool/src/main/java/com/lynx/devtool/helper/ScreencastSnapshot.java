// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.PixelCopy;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import com.lynx.devtoolwrapper.DevToolOverlayDelegate;
import com.lynx.devtoolwrapper.ScreenshotMode;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import java.lang.reflect.Field;
import java.util.ArrayList;

/** UI-thread snapshot preparation; PixelCopy never waits on the UI thread. */
final class ScreencastSnapshot {
  static final String ISOLATED_VIEW = "isolated-view";
  /** Session-owned scratch space; release after the outstanding capture completes. */
  static final class Buffers {
    private Bitmap mLayer;

    Bitmap obtain(int width, int height) {
      if (mLayer == null || mLayer.getWidth() != width || mLayer.getHeight() != height) {
        release();
        mLayer = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
      }
      return mLayer;
    }

    void release() {
      if (mLayer != null) {
        mLayer.recycle();
        mLayer = null;
      }
    }
  }

  interface Callback {
    void complete(Bitmap bitmap, int sourceWidth, int sourceHeight);
  }

  private static final Handler MAIN = new Handler(Looper.getMainLooper());
  private final ArrayList<View> mViews = new ArrayList<>();
  private final ArrayList<Rect> mBounds = new ArrayList<>();
  private final Callback mCallback;
  private final int mSourceWidth;
  private final int mSourceHeight;
  private final Bitmap mBitmap;
  private final Canvas mCanvas;
  private final Buffers mBuffers;
  private final boolean mIsolated;
  private final Rect mCrop;

  static void capture(View view, String mode, int maxWidth, int maxHeight, Bitmap reusable,
      Buffers buffers, Rect crop, Callback callback) {
    ScreencastSnapshot snapshot;
    try {
      snapshot = new ScreencastSnapshot(
          view, mode, maxWidth, maxHeight, reusable, buffers, crop, callback);
    } catch (RuntimeException error) {
      LLog.w("Screencast", "Cannot prepare snapshot: " + error);
      if (reusable != null) {
        reusable.recycle();
      }
      callback.complete(null, 0, 0);
      return;
    }
    snapshot.copyNext(0);
  }

  private ScreencastSnapshot(View view, String mode, int maxWidth, int maxHeight, Bitmap reusable,
      Buffers buffers, Rect crop, Callback callback) {
    if (view == null || !view.isAttachedToWindow() || view.getWidth() <= 0
        || view.getHeight() <= 0) {
      throw new IllegalArgumentException("View is not ready");
    }
    mCallback = callback;
    mBuffers = buffers;
    mIsolated = ISOLATED_VIEW.equals(mode);
    mCrop = crop;
    if (mIsolated || ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW.equals(mode)) {
      mSourceWidth = view.getWidth();
      mSourceHeight = view.getHeight();
      mViews.add(view);
      mBounds.add(new Rect(0, 0, mSourceWidth, mSourceHeight));
    } else {
      if (crop != null) {
        mSourceWidth = crop.width();
        mSourceHeight = crop.height();
      } else {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager manager =
            (WindowManager) view.getContext().getSystemService(Context.WINDOW_SERVICE);
        if (manager == null) {
          throw new IllegalStateException("No display");
        }
        manager.getDefaultDisplay().getRealMetrics(metrics);
        mSourceWidth = metrics.widthPixels;
        mSourceHeight = metrics.heightPixels;
      }
      addView(view.getRootView());
      ArrayList<Dialog> overlays = DevToolOverlayDelegate.getInstance().getGlobalOverlayView();
      if (overlays != null) {
        for (int i = overlays.size() - 1; i >= 0; i--) {
          Dialog dialog = overlays.get(i);
          if (dialog != null && dialog.isShowing() && dialog.getWindow() != null) {
            addView(dialog.getWindow().getDecorView());
          }
        }
      }
    }
    float scale =
        Math.min(1, Math.min(maxWidth / (float) mSourceWidth, maxHeight / (float) mSourceHeight));
    int width = Math.max(1, Math.round(mSourceWidth * scale));
    int height = Math.max(1, Math.round(mSourceHeight * scale));
    if (reusable != null && !reusable.isRecycled() && reusable.getWidth() == width
        && reusable.getHeight() == height) {
      mBitmap = reusable;
    } else {
      if (reusable != null) {
        reusable.recycle();
      }
      mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    }
    mBitmap.eraseColor(Color.WHITE);
    mCanvas = new Canvas(mBitmap);
  }

  private void addView(View view) {
    if (view == null || !view.isAttachedToWindow() || view.getWidth() <= 0 || view.getHeight() <= 0
        || mViews.contains(view)) {
      return;
    }
    int[] location = new int[2];
    view.getLocationOnScreen(location);
    mViews.add(view);
    Rect bounds = new Rect(
        location[0], location[1], location[0] + view.getWidth(), location[1] + view.getHeight());
    if (mCrop != null) {
      bounds.offset(-mCrop.left, -mCrop.top);
      if (!bounds.intersects(0, 0, mSourceWidth, mSourceHeight)) {
        return;
      }
    }
    mViews.add(view);
    mBounds.add(bounds);
  }

  private void copyNext(int index) {
    if (index == mViews.size()) {
      mCallback.complete(mBitmap, mSourceWidth, mSourceHeight);
      return;
    }
    View view = mViews.get(index);
    Rect bounds = mBounds.get(index);
    float scaleX = mBitmap.getWidth() / (float) mSourceWidth;
    float scaleY = mBitmap.getHeight() / (float) mSourceHeight;
    Rect destination = new Rect(Math.round(bounds.left * scaleX), Math.round(bounds.top * scaleY),
        Math.round(bounds.right * scaleX), Math.round(bounds.bottom * scaleY));
    try {
      if (destination.isEmpty()) {
        MAIN.post(() -> copyNext(index + 1));
        return;
      }
      // Window readback includes siblings and overlays. Isolated previews draw only this view.
      if (!mIsolated && Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
          && LynxEnv.inst().isPixelCopyEnabled()) {
        View root = view.getRootView();
        Object viewRoot = root.getParent();
        Class<?> rootClass = viewRoot.getClass();
        Field surfaceField = rootClass.getDeclaredField("mSurface");
        surfaceField.setAccessible(true);
        Surface surface = (Surface) surfaceField.get(viewRoot);
        int[] location = new int[2];
        int[] rootLocation = new int[2];
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
          view.getLocationInSurface(location);
          root.getLocationInSurface(rootLocation);
        } else {
          view.getLocationInWindow(location);
          root.getLocationInWindow(rootLocation);
          Field attributesField = rootClass.getDeclaredField("mWindowAttributes");
          attributesField.setAccessible(true);
          Object attributes = attributesField.get(viewRoot);
          Field insetsField = attributes.getClass().getField("surfaceInsets");
          Rect insets = (Rect) insetsField.get(attributes);
          location[0] += insets.left;
          location[1] += insets.top;
          rootLocation[0] += insets.left;
          rootLocation[1] += insets.top;
        }
        Rect source = new Rect(
            location[0], location[1], location[0] + bounds.width(), location[1] + bounds.height());
        if (mCrop != null
            && !source.intersect(location[0] - bounds.left, location[1] - bounds.top,
                location[0] - bounds.left + mSourceWidth,
                location[1] - bounds.top + mSourceHeight)) {
          MAIN.post(() -> copyNext(index + 1));
          return;
        }
        if (!source.intersect(rootLocation[0], rootLocation[1], rootLocation[0] + root.getWidth(),
                rootLocation[1] + root.getHeight())) {
          MAIN.post(() -> copyNext(index + 1));
          return;
        }
        int destinationX = mCrop == null ? destination.left : 0;
        int destinationY = mCrop == null ? destination.top : 0;
        int sourceX = mCrop == null ? location[0] : location[0] - bounds.left;
        int sourceY = mCrop == null ? location[1] : location[1] - bounds.top;
        Rect clipped = new Rect(destinationX + Math.round((source.left - sourceX) * scaleX),
            destinationY + Math.round((source.top - sourceY) * scaleY),
            destinationX + Math.round((source.right - sourceX) * scaleX),
            destinationY + Math.round((source.bottom - sourceY) * scaleY));
        if (clipped.isEmpty()) {
          MAIN.post(() -> copyNext(index + 1));
          return;
        }
        // A fullscreen root can be copied straight into the session-owned output buffer.
        boolean direct =
            index == 0 && clipped.equals(new Rect(0, 0, mBitmap.getWidth(), mBitmap.getHeight()));
        Bitmap target = direct ? mBitmap : mBuffers.obtain(clipped.width(), clipped.height());
        PixelCopy.request(surface, source, target, result -> {
          if (result == PixelCopy.SUCCESS) {
            if (!direct) {
              mCanvas.drawBitmap(target, null, clipped, null);
            }
          }
          if (result != PixelCopy.SUCCESS) {
            // Never send a partially composed frame or advance its reference.
            fail();
          } else {
            copyNext(index + 1);
          }
        }, MAIN);
        return;
      }
      draw(view, destination, bounds);
    } catch (ReflectiveOperationException | RuntimeException error) {
      // Hidden surface access may be unavailable. View.draw remains a bounded-size fallback.
      try {
        draw(view, destination, bounds);
      } catch (RuntimeException drawError) {
        fail();
        return;
      }
    }
    MAIN.post(() -> copyNext(index + 1));
  }

  private void draw(View view, Rect destination, Rect bounds) {
    int save = mCanvas.save();
    try {
      mCanvas.clipRect(destination);
      mCanvas.translate(destination.left, destination.top);
      mCanvas.scale(destination.width() / (float) bounds.width(),
          destination.height() / (float) bounds.height());
      view.draw(mCanvas);
    } finally {
      mCanvas.restoreToCount(save);
    }
  }

  private void fail() {
    mBitmap.recycle();
    mCallback.complete(null, 0, 0);
  }
}
