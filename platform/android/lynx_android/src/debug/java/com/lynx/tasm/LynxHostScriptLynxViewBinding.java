// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.base.CalledByNative;
import java.lang.ref.WeakReference;
import org.json.JSONArray;

/** Runtime-local, non-owning LynxView binding used by the debug Host Script Runtime. */
@Keep
@RestrictTo(RestrictTo.Scope.LIBRARY)
final class LynxHostScriptLynxViewBinding {
  private final Handler mUiHandler = new Handler(Looper.getMainLooper());

  private final LynxHostScriptRuntimeBinding mRuntimeBinding;
  private final LynxViewClient mViewClient;
  private volatile WeakReference<LynxView> mBoundView = new WeakReference<>(null);
  private volatile boolean mViewDestroyed;

  LynxHostScriptLynxViewBinding(LynxHostScriptRuntimeBinding runtimeBinding) {
    mRuntimeBinding = runtimeBinding;
    mViewClient = new LynxViewClient() {
      @Override
      public void onLoadSuccess() {
        notifyEvent("loadSuccess", 0, null);
      }

      @Override
      public void onFirstScreen() {
        notifyEvent("firstScreen", 0, null);
      }

      @Override
      public void onPageUpdate() {
        notifyEvent("pageUpdate", 0, null);
      }

      @Override
      public void onDataUpdated() {
        notifyEvent("dataUpdated", 0, null);
      }

      @Override
      public void onUpdateDataWithoutChange() {
        notifyEvent("dataUpdated", 0, null);
      }

      @Override
      public void onReceivedError(LynxError error) {
        if (error != null) {
          notifyEvent("error", error.getErrorCode(), error.getSummaryMessage());
        }
      }

      @Override
      public void onDestroy() {
        mViewDestroyed = true;
        mBoundView.clear();
        notifyEvent("destroyed", 0, null);
      }
    };
  }

  boolean bindView(LynxView view) {
    synchronized (mRuntimeBinding) {
      long nativePtr = mRuntimeBinding.nativePtr();
      if (nativePtr == 0 || view == null || mBoundView.get() != null || mViewDestroyed) {
        return false;
      }
      // Publish the Java target before native binding wakes JS ready waiters.
      mBoundView = new WeakReference<>(view);
      view.addLynxViewClient(mViewClient);
      if (!nativeBindView(nativePtr, this, view)) {
        view.removeLynxViewClient(mViewClient);
        mBoundView.clear();
        return false;
      }
      return true;
    }
  }

  void destroy() {
    LynxView view = mBoundView.get();
    if (view != null) {
      view.removeLynxViewClient(mViewClient);
    }
    mViewDestroyed = true;
    mBoundView.clear();
    synchronized (mRuntimeBinding) {
      long nativePtr = mRuntimeBinding.nativePtr();
      if (nativePtr != 0) {
        nativeInvalidate(nativePtr);
      }
    }
  }

  private void notifyEvent(String event, int code, @Nullable String message) {
    synchronized (mRuntimeBinding) {
      long nativePtr = mRuntimeBinding.activeNativePtr();
      if (nativePtr != 0) {
        nativeNotify(nativePtr, event, code, message);
      }
    }
  }

  private boolean isDispatchTargetAvailable(@Nullable LynxView target) {
    return mRuntimeBinding.activeNativePtr() != 0 && !mViewDestroyed && target != null
        && mBoundView.get() == target;
  }

  private interface ViewAction {
    void run(LynxView view) throws Exception;
  }

  private boolean enqueue(LynxView view, ViewAction action) {
    synchronized (mRuntimeBinding) {
      if (!isDispatchTargetAvailable(view)) {
        return false;
      }
      WeakReference<LynxView> weakView = new WeakReference<>(view);
      return mUiHandler.post(() -> {
        LynxView target = weakView.get();
        synchronized (mRuntimeBinding) {
          if (mRuntimeBinding.activeNativePtr() == 0) {
            return;
          }
          if (!isDispatchTargetAvailable(target)) {
            mViewDestroyed = true;
            mBoundView.clear();
            notifyEvent("destroyed", 0, null);
            return;
          }
        }
        try {
          action.run(target);
        } catch (Throwable throwable) {
          notifyEvent("error", -1, throwable.toString());
        }
      });
    }
  }

  @CalledByNative
  private static boolean enqueueLoadTemplate(LynxHostScriptLynxViewBinding binding, LynxView view,
      byte[] templateBytes, String url, String initialDataJson, String globalPropsJson,
      String processorName, boolean readOnly) {
    return binding.enqueue(view, target -> {
      LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
      builder.setUrl(url);
      if (templateBytes != null && templateBytes.length > 0) {
        builder.setBinaryData(templateBytes);
      }
      TemplateData initialData = createTemplateData(initialDataJson, processorName, readOnly);
      if (initialData != null) {
        builder.setInitialData(initialData);
      }
      TemplateData globalProps = createTemplateData(globalPropsJson, null, false);
      if (globalProps != null) {
        builder.setGlobalProps(globalProps);
      }
      target.loadTemplate(builder.build());
    });
  }

  @CalledByNative
  private static boolean enqueueLoadSSR(LynxHostScriptLynxViewBinding binding, LynxView view,
      byte[] ssrBytes, String url, String initialDataJson) {
    return binding.enqueue(view, target -> {
      target.renderSSR(ssrBytes, url, createTemplateData(initialDataJson, null, false));
    });
  }

  @CalledByNative
  private static boolean enqueueHydrateSSR(LynxHostScriptLynxViewBinding binding, LynxView view,
      byte[] templateBytes, String url, String initialDataJson) {
    return binding.enqueue(view, target -> {
      target.ssrHydrate(templateBytes, url, createTemplateData(initialDataJson, null, false));
    });
  }

  @CalledByNative
  private static boolean enqueueUpdateMetaData(LynxHostScriptLynxViewBinding binding, LynxView view,
      String dataJson, String globalPropsJson) {
    return binding.enqueue(view, target -> {
      LynxUpdateMeta.Builder builder = new LynxUpdateMeta.Builder();
      if (dataJson != null) {
        builder.setUpdatedData(createTemplateData(dataJson, null, false));
      }
      if (globalPropsJson != null) {
        builder.setUpdatedGlobalProps(createTemplateData(globalPropsJson, null, false));
      }
      target.updateMetaData(builder.build());
    });
  }

  @CalledByNative
  private static boolean enqueueSetGlobalProps(
      LynxHostScriptLynxViewBinding binding, LynxView view, String globalPropsJson) {
    return binding.enqueue(
        view, target -> target.setGlobalProps(createTemplateData(globalPropsJson, null, false)));
  }

  @CalledByNative
  private static boolean enqueueReloadTemplate(LynxHostScriptLynxViewBinding binding, LynxView view,
      String dataJson, String globalPropsJson) {
    return binding.enqueue(view, target -> {
      TemplateData data = createTemplateData(dataJson, null, false);
      TemplateData globalProps = createTemplateData(globalPropsJson, null, false);
      target.reloadTemplate(data == null ? TemplateData.empty() : data, globalProps);
    });
  }

  @CalledByNative
  private static boolean enqueueSendGlobalEvent(
      LynxHostScriptLynxViewBinding binding, LynxView view, String name, String paramsJson) {
    return binding.enqueue(view, target -> {
      JSONArray jsonArray = new JSONArray(paramsJson);
      target.sendGlobalEvent(name, JavaOnlyArray.from(TemplateData.jsonArrayToList(jsonArray)));
    });
  }

  @Nullable
  private static TemplateData createTemplateData(
      @Nullable String json, @Nullable String processorName, boolean readOnly) {
    if (json == null || json.isEmpty()) {
      return null;
    }
    TemplateData data = TemplateData.fromString(json);
    if (processorName != null && !processorName.isEmpty()) {
      data.markState(processorName);
    }
    if (readOnly) {
      data.markReadOnly();
    }
    return data;
  }

  private static native boolean nativeBindView(
      long nativePtr, LynxHostScriptLynxViewBinding binding, LynxView view);
  private static native void nativeInvalidate(long nativePtr);
  private static native void nativeNotify(long nativePtr, String event, int code, String message);
}
