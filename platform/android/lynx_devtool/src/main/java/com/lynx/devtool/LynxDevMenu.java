// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;
import java.util.LinkedHashMap;

public class LynxDevMenu {
  private final static long PRESS_RANGE = 1000;
  private final static long CLICK_RANGE = 15;

  private final WeakReference<LynxInspectorOwner> mOwner;
  private float mStartX;
  private float mStartY;
  private float mPointX;
  private float mPointY;
  private AlertDialog mDevOptionsDialog;
  private Handler mLongPressHandler;
  private WeakReference<LynxView> mLynxView;
  public LynxDevMenu(LynxInspectorOwner owner, @Nullable LynxView lynxView) {
    mOwner = new WeakReference<>(owner);
    mLynxView = new WeakReference<>(lynxView);
    mDevOptionsDialog = null;
    mLongPressHandler = new Handler(Looper.getMainLooper());
  }
  public void attach(LynxView lynxView) {
    mLynxView = new WeakReference<>(lynxView);
  }
  public void showDevOptionsDialog() {
    if (mDevOptionsDialog != null) {
      return;
    }

    final LynxInspectorOwner owner = mOwner.get();
    if (owner == null) {
      return;
    }

    String message = "";
    if (owner.isDebugging()) {
      message = "IP: " + owner.getHttpServerIp() + "\nPort: " + owner.getHttpServerPort();
      message += "\nSession ID: " + owner.getSessionID();
    }

    LinkedHashMap<String, DevOptionHandler> options = new LinkedHashMap<>();
    if (!message.isEmpty()) {
      options.put(message, new DevOptionHandler() {
        @Override
        public void onOptionSelected() {}
      });
    }
    options.put("Reload", new DevOptionHandler() {
      @Override
      public void onOptionSelected() {
        owner.reload(false);
      }
    });

    final DevOptionHandler[] optionHandlers = options.values().toArray(new DevOptionHandler[0]);

    AlertDialog.Builder builder = new AlertDialog.Builder(owner.getLynxView().getContext());
    builder.setTitle("Lynx Debug Menu");
    builder.setItems(
        options.keySet().toArray(new String[0]), new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialogInterface, int i) {
            optionHandlers[i].onOptionSelected();
            mDevOptionsDialog = null;
          }
        });
    builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
      @Override
      public void onCancel(DialogInterface dialogInterface) {
        mDevOptionsDialog = null;
      }
    });
    mDevOptionsDialog = builder.create();
    mDevOptionsDialog.show();
  }

  public boolean onRootViewTouchEvent(MotionEvent event) {
    switch (event.getAction()) {
      case MotionEvent.ACTION_DOWN:
        return onActionDown(event);
      case MotionEvent.ACTION_MOVE:
        return onActionMove(event);
      case MotionEvent.ACTION_UP:
        return onActionUp(event);
      default:
        return onActionCancel(event);
    }
  }

  private boolean isInViewArea(float x, float y) {
    LynxView lynxView = mLynxView.get();
    if (lynxView == null) {
      return false;
    }
    try {
      Rect r = new Rect();
      lynxView.getLocalVisibleRect(r);
      if (x > r.left && x < r.right && y > r.top && y < r.bottom) {
        return true;
      } else {
        return false;
      }
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return false;
  }

  private boolean onActionDown(MotionEvent event) {
    mStartX = event.getX();
    mStartY = event.getY();
    if (isInViewArea(mStartX, mStartY)) {
      mLongPressHandler.postDelayed(new Runnable() {
        @Override
        public void run() {
          try {
            showDevOptionsDialog();
          } catch (Throwable t) {
            t.printStackTrace();
          }
        }
      }, PRESS_RANGE);
    }
    return false;
  }

  private boolean onActionMove(MotionEvent event) {
    mPointX = event.getX();
    mPointY = event.getY();
    if (!isInViewArea(mPointX, mPointY) || Math.abs(mPointX - mStartX) > CLICK_RANGE
        || Math.abs(mPointY - mStartY) > CLICK_RANGE) {
      this.mLongPressHandler.removeCallbacksAndMessages(null);
    }
    return false;
  }

  private boolean onActionUp(MotionEvent event) {
    this.mLongPressHandler.removeCallbacksAndMessages(null);
    return false;
  }

  private boolean onActionCancel(MotionEvent event) {
    this.mLongPressHandler.removeCallbacksAndMessages(null);
    return false;
  }
}
