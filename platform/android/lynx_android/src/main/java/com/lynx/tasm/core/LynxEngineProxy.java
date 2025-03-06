// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.Runnable;
import java.nio.ByteBuffer;

public final class LynxEngineProxy {
  private static final String TAG = "LynxEngineProxy";
  public long mNativePtr;

  public LynxEngineProxy(long nativeCreator) {
    mNativePtr = nativeCreate(nativeCreator);
  }

  public void destroy() {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        nativeDestroy(mNativePtr);
        mNativePtr = 0;
      }
    });
  }

  public void dispatchTaskToLynxEngine(Runnable runnable) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "DispatchTaskToLynxEngine failed since mNativePtr is null");
          return;
        }

        nativeDispatchTaskToLynxEngine(mNativePtr, runnable);
      }
    });
  }

  public void sendTouchEvent(LynxTouchEvent event) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
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
    });
  }

  public void sendMultiTouchEvent(LynxTouchEvent event) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
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
    });
  }

  public void sendCustomEvent(LynxCustomEvent event) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
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
    });
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
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
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
    });
  }

  public void onPseudoStatusChanged(final int id, final int preStatus, final int currentStatus) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "OnPseudoStatusChanged failed since mNativePtr is null");
          return;
        }
        nativeOnPseudoStatusChanged(mNativePtr, id, preStatus, currentStatus);
      }
    });
  }

  public void invokeLepusApiCallback(int callbackID, String entryName, Object data) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (mNativePtr == 0) {
          LLog.e(TAG, "invokeLepusApiCallback failed since mNativePtr is null");
          return;
        }
        nativeInvokeLepusApiCallback(mNativePtr, callbackID, entryName, data);
      }
    });
  }

  @CalledByNative
  private static void executeRunnable(Runnable runnable) {
    runnable.run();
  }

  private native long nativeCreate(long nativePtr);

  private native void nativeDestroy(long nativePtr);

  private native void nativeDispatchTaskToLynxEngine(long nativePtr, Runnable runnable);

  private native void nativeSendTouchEvent(long nativePtr, String name, int tag, float clientX,
      float clientY, float pageX, float pageY, float viewX, float viewY, long timestamp);

  private native void nativeSendMultiTouchEvent(
      long nativePtr, String name, ByteBuffer params, int length, long timestamp);

  private native void nativeSendCustomEvent(
      long nativePtr, String name, int tag, ByteBuffer params, int length, String paramsName);

  private native void nativeSendGestureEvent(
      long nativePtr, String name, int tag, int gestureId, ByteBuffer params, int length);

  private native void nativeOnPseudoStatusChanged(
      long nativePtr, int id, int preStatus, int currentStatus);

  private native void nativeInvokeLepusApiCallback(
      long nativePtr, int callbackID, String entryName, Object data);
}
