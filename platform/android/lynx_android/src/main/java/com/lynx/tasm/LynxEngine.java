// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import java.lang.ref.WeakReference;
import java.util.Deque;

/**
 * Do not make this class public until a proxy class is added to decouple it from the Pool.
 */
class LynxEngine {
  enum LynxEngineState {
    UN_LOADED,
    READY_BE_REUSED,
    ON_REUSING,
    FREEZED,
    DESTROYED,
  }

  private static final String TAG = "LynxEngineWrapper";

  private WeakReference<LynxTemplateRender> mLynxTemplateRender;

  private long mNativePtr;
  private TemplateBundle mTemplateBundle;
  private LynxEngineState mLynxEngineState = LynxEngineState.UN_LOADED;
  private WeakReference<Deque<LynxEngine>> mLynxEngineWrapperQueue;

  private ThreadStrategyForRendering mThreadStrategy;
  private ILynxUIRenderer mLynxUIRenderer;
  private TasmPlatformInvoker mTasmPlatformInvoker;
  // LynxContext or PageConfig
  private PageConfig mPageConfig;

  public LynxEngine(@NonNull TemplateBundle templateBundle, LynxTemplateRender templateRender) {
    /**
     * 1. Check if there is an available EngineWrapper and retrieve it from the cache.
     *  1.1 If no EngineWrapper is available, create one and obtain the nativePtr of the
     * EngineWrapper.
     * 2. Create the Shell.
     *  2.1 Pass in the nativePtr of the EngineWrapper to complete the relationship binding between
     * the Shell and the Wrapper. 2.2 Create the PlatformContext and bind it to the Wrapper.
     * 3. Load the template card.
     *  3.1 Unbind and add to the EnginePool queue.
     * 4. When the card is destroyed, the Engine is not destroyed. It waits for the EnginePool to
     * request destruction.
     */
    // Note: At this time, the Engine object does not exist yet because it still depends on the
    // complete information of the Shell for creation.
    //       Therefore, the object information needs to be provided to the Wrapper for management
    //       after the Shell is created.
    mNativePtr = nativeCreate();
    mLynxTemplateRender = new WeakReference<>(templateRender);
    mTemplateBundle = templateBundle;
    mThreadStrategy = templateRender.getThreadStrategyForRendering();
  }

  public ThreadStrategyForRendering getThreadStrategy() {
    return mThreadStrategy;
  }

  public void setThreadStrategy(ThreadStrategyForRendering threadStrategy) {
    this.mThreadStrategy = threadStrategy;
  }

  public TemplateBundle getTemplateBundle() {
    return mTemplateBundle;
  }

  public long getNativePtr() {
    return mNativePtr;
  }
  public void setQueueRefFromPool(Deque<LynxEngine> lynxEngineQueue) {
    this.mLynxEngineWrapperQueue = new WeakReference<>(lynxEngineQueue);
  }

  public ILynxUIRenderer getLynxUIRenderer() {
    return mLynxUIRenderer;
  }

  public void setLynxUIRenderer(ILynxUIRenderer lynxUIRenderer) {
    this.mLynxUIRenderer = lynxUIRenderer;
  }

  public TasmPlatformInvoker getTasmPlatformInvoker() {
    return mTasmPlatformInvoker;
  }

  public void setTasmPlatformInvoker(TasmPlatformInvoker tasmPlatformInvoker) {
    this.mTasmPlatformInvoker = tasmPlatformInvoker;
  }

  public PageConfig getPageConfig() {
    return mPageConfig;
  }

  public void setPageConfig(PageConfig pageConfig) {
    this.mPageConfig = pageConfig;
  }

  public void detachFromLynxView() {
    LynxTemplateRender lynxTemplateRender = mLynxTemplateRender.get();
    if (mNativePtr != 0 && lynxTemplateRender != null) {
      lynxTemplateRender.detachLynxEngineWrapper();
      mLynxTemplateRender.clear();
    }
  }

  public synchronized void updateLynxEngineState(LynxEngineState lynxEngineState) {
    LLog.i(TAG, this + "updateLynxEngineState:" + lynxEngineState.name());
    if (this.mLynxEngineState == LynxEngineState.DESTROYED) {
      if (lynxEngineState != LynxEngineState.DESTROYED) {
        LLog.e(TAG, "lynxEngineState has been destroyed." + this);
      }
      return;
    }
    this.mLynxEngineState = lynxEngineState;
  }

  public synchronized boolean hasLoaded() {
    return mLynxEngineState != LynxEngineState.UN_LOADED;
  }

  public synchronized boolean canReused() {
    return mLynxEngineState == LynxEngineState.READY_BE_REUSED;
  }

  public synchronized boolean tryBeReusing() {
    if (mLynxEngineState == LynxEngineState.READY_BE_REUSED) {
      updateLynxEngineState(LynxEngineState.ON_REUSING);
      return true;
    }
    return false;
  }

  public void attachCurrentTemplateRender(LynxTemplateRender templateRender) {
    mLynxTemplateRender = new WeakReference<>(templateRender);
  }

  public void registerLynxEngineReused() {
    updateLynxEngineState(LynxEngineState.READY_BE_REUSED);
    LynxEnginePool.getInstance().registerReuseEngineWrapper(this);
  }

  public void destroy() {
    updateLynxEngineState(LynxEngineState.DESTROYED);
    LynxEnginePool.getInstance().delete(this);
    if (mNativePtr != 0) {
      long nativePtr = mNativePtr;
      mNativePtr = 0;
      nativeDestroyEngine(nativePtr);
    }
  }

  private native long nativeCreate();

  private native long nativeDetachEngine(long nativePtr);

  private native void nativeDestroyEngine(long nativePtr);

  @Override
  public String toString() {
    return "LynxEngineWrapper{"
        + "mNativePtr=" + mNativePtr + ", mLynxEngineState=" + mLynxEngineState + '}';
  }
}
