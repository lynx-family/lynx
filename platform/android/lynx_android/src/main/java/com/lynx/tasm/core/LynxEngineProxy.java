// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import android.os.Handler;
import android.os.Looper;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import java.nio.ByteBuffer;

public final class LynxEngineProxy {
  private static final String TAG = "LynxEngineProxy";
  public long mNativePtr;
  private Handler mHandler;

  public LynxEngineProxy(long nativeCreator) {
    mNativePtr = nativeCreate(nativeCreator);
    mHandler = new Handler(Looper.getMainLooper());
  }

  public void destroy() {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        nativeDestroy(mNativePtr);
        mNativePtr = 0;
      }
    };
    run(runnable);
  }

  private void run(Runnable runnable) {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      runnable.run();
    } else {
      mHandler.post(runnable);
    }
  }

  public void sendTouchEvent(LynxTouchEvent event) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "sendTouchEvent failed since mNativePtr is null");
          return;
        }

        LynxTouchEvent.Point clientPoint = event.getClientPoint();
        LynxTouchEvent.Point pagePoint = event.getPagePoint();
        LynxTouchEvent.Point viewPoint = event.getViewPoint();
        nativeSendTouchEvent(mNativePtr, event.getName(), event.getTag(), clientPoint.getX(),
            clientPoint.getY(), pagePoint.getX(), pagePoint.getY(), viewPoint.getX(),
            viewPoint.getY(), event.getTimestamp());
      }
    };
    run(runnable);
  }

  public void sendMultiTouchEvent(LynxTouchEvent event) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "sendMultiTouchEvent failed since mNativePtr is null");
          return;
        }

        ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(event.getUITouchMap());
        int length = buffer == null ? 0 : buffer.position();
        nativeSendMultiTouchEvent(
            mNativePtr, event.getName(), buffer, length, event.getTimestamp());
      }
    };
    run(runnable);
  }

  public void sendCustomEvent(LynxCustomEvent event) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "SendCustomEvent failed since mNativePtr is null");
          return;
        }

        ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(event.eventParams());
        int length = buffer == null ? 0 : buffer.position();
        String paramsName = event.paramsName();
        nativeSendCustomEvent(
            mNativePtr, event.getName(), event.getTag(), buffer, length, paramsName);
      }
    };
    run(runnable);
  }

  /**
   * Sends a gesture event with specified details asynchronously.
   *
   * @param name The name of the gesture event.
   * @param tag An additional tag for the event.
   * @param gestureId The identifier of the specific gesture.
   * @param params A ByteBuffer containing event parameters.
   * @param length The length of the data in the ByteBuffer.
   */
  public void sendGestureEvent(
      final String name, final int tag, int gestureId, final ByteBuffer params, final int length) {
    // Create a Runnable to perform the gesture event sending.
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        // Check if the native pointer is valid.
        if (mNativePtr == 0) {
          LLog.e(TAG, "sendGestureEvent failed since mNativePtr is null");
          return;
        }

        // Invoke the native method to send the gesture event.
        nativeSendGestureEvent(mNativePtr, name, tag, gestureId, params, length);
      }
    };

    // Execute the Runnable asynchronously.
    run(runnable);
  }

  public void onPseudoStatusChanged(final int id, final int preStatus, final int currentStatus) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "OnPseudoStatusChanged failed since mNativePtr is null");
          return;
        }
        nativeOnPseudoStatusChanged(mNativePtr, id, preStatus, currentStatus);
      }
    };
    run(runnable);
  }

  public void invokeLepusApiCallback(int callbackID, String entryName, Object data) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "invokeLepusApiCallback failed since mNativePtr is null");
          return;
        }
        nativeInvokeLepusApiCallback(mNativePtr, callbackID, entryName, data);
      }
    };
    run(runnable);
  }

  private native long nativeCreate(long nativeCreator);
  private native void nativeDestroy(long nativePtr);

  private native void nativeSendTouchEvent(long ptr, String name, int tag, float clientX,
      float clientY, float pageX, float pageY, float viewX, float viewY, long timestamp);
  private native void nativeSendMultiTouchEvent(
      long ptr, String name, ByteBuffer params, int length, long timestamp);
  private native void nativeSendCustomEvent(
      long ptr, String name, int tag, ByteBuffer params, int length, String paramsName);

  private native void nativeSendGestureEvent(
      long ptr, String name, int tag, int gestureId, ByteBuffer params, int length);

  private native void nativeOnPseudoStatusChanged(
      long ptr, int id, int preStatus, int currentStatus);

  private native void nativeInvokeLepusApiCallback(
      long nativePtr, int callbackID, String entryName, Object data);
}
