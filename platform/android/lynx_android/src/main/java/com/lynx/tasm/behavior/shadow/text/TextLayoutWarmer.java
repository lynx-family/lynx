// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static android.os.Process.THREAD_PRIORITY_BACKGROUND;

import android.graphics.Canvas;
import android.graphics.Picture;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.text.Layout;
import java.lang.ref.WeakReference;

public class TextLayoutWarmer extends HandlerThread {
  private static final String TAG = "TextLayoutWarmer";
  private volatile Handler mHandler;

  private static class Holder {
    private static TextLayoutWarmer warmer = new TextLayoutWarmer();
  }

  public static TextLayoutWarmer warmer() {
    return Holder.warmer;
  }

  private TextLayoutWarmer() {
    super(TAG, THREAD_PRIORITY_BACKGROUND);
    start();
  }

  @Override
  protected void onLooperPrepared() {
    super.onLooperPrepared();
    mHandler = new WarmerHandler(Looper.myLooper());
  }

  /**
   * Schedules a {@link Layout} to be drawn in the background. This warms up the Glyph cache for
   * that {@link Layout}.
   */
  public void warmLayout(Layout layout) {
    if (mHandler != null) {
      mHandler.obtainMessage(WarmerHandler.WARM_LAYOUT, new WeakReference<>(layout)).sendToTarget();
    }
  }

  private static final class WarmerHandler extends android.os.Handler {
    public static final int WARM_LAYOUT = 0;

    private final Picture mPicture;

    private WarmerHandler(Looper looper) {
      super(looper);

      Picture picture;
      try {
        picture = new Picture();
      } catch (RuntimeException e) {
        picture = null;
      }

      mPicture = picture;
    }

    @Override
    @SuppressWarnings("unchecked")
    public void handleMessage(Message msg) {
      if (mPicture == null) {
        return;
      }
      try {
        if (msg.what == WARM_LAYOUT) {
          final Layout layout = ((WeakReference<Layout>) msg.obj).get();
          if (layout == null) {
            return;
          }
          final Canvas canvas = mPicture.beginRecording(layout.getWidth(), layout.getHeight());
          layout.draw(canvas);
          mPicture.endRecording();
        }
      } catch (Exception e) {
        // Nothing to do here. This is a best effort. No real problem if it fails.
      }
    }
  }
}
