// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import androidx.annotation.Nullable;
import com.lynx.devtool.recorder.LynxRecorderActivity;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.devtoolwrapper.LynxDevtoolGlobalHelper;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.explorer.scan.QRScanActivity;
import com.lynx.explorer.shell.TemplateDispatcher;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.LynxEnv;
import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

public class LynxModuleAdapter {
  private Context mContext;
  private Handler mHandler;
  private WeakReference<Activity> mTopActivity = new WeakReference<>(null);

  private LynxDevtoolCardListener mListener = new LynxDevtoolCardListener() {
    @Override
    public void open(String url) {
      startFromUrlSingleTop(url);
    }
  };

  private static final int OPEN_SCHEMA = 0;
  private static final int OPEN_SCAN = 1;
  private static final LynxModuleAdapter sInstance = new LynxModuleAdapter();

  public static LynxModuleAdapter getInstance() {
    return sInstance;
  }

  public void Init(Context context) {
    mContext = context;
    mHandler = new Handler(Looper.getMainLooper()) {
      @Override
      public void handleMessage(Message msg) {
        switch (msg.what) {
          case OPEN_SCAN:
            startQRScanActivity();
            break;
          case OPEN_SCHEMA:
            startFromUrl((String) msg.obj);
            break;
          default:
        }
      }
    };
    LynxEnv.inst().registerModule("ExplorerModule", ExplorerModule.class);

    LynxDevtoolGlobalHelper.getInstance().registerCardListener(mListener);
  }

  public void openScan() {
    mHandler.sendEmptyMessage(OPEN_SCAN);
  }

  public void openSchema(String url) {
    Message msg = Message.obtain();
    msg.obj = url;
    msg.what = OPEN_SCHEMA;
    mHandler.sendMessage(msg);
  }

  public void onActivityResumed(Activity activity) {
    mTopActivity = new WeakReference<>(activity);
  }

  public void onActivityPaused(Activity activity) {
    Activity topActivity = mTopActivity.get();
    if (topActivity == activity) {
      mTopActivity.clear();
    }
  }

  public boolean hasClosablePage() {
    Activity activity = mTopActivity.get();
    return isClosableExplorerActivity(activity) && !isActivityUnavailable(activity);
  }

  public void openSchemaSync(String url) {
    executeOnMainThread(() -> {
      startFromUrl(url);
      return null;
    });
  }

  public void closeCurrentPageSync() {
    executeOnMainThread(() -> {
      Activity activity = mTopActivity.get();
      if (!isClosableExplorerActivity(activity)) {
        throw new IllegalStateException("No closable Explorer page is active.");
      }
      if (isActivityUnavailable(activity)) {
        throw new IllegalStateException("Current Explorer page is already closing.");
      }
      activity.finish();
      return null;
    });
  }

  public void setThreadMode(int threadMode) {
    LynxSettingManager.getInstance().setThreadStrategy(threadMode);
  }

  public void setEnablePresetSize(boolean enablePresetSize) {
    LynxSettingManager.getInstance().setEnablePresetSize(enablePresetSize);
  }

  void enableRenderNode(boolean enableRenderNode) {
    LynxSettingManager.getInstance().enableRenderNode(enableRenderNode);
  }

  WritableMap getSettingInfo() {
    WritableMap map = new JavaOnlyMap();
    SettingInfo info = LynxSettingManager.getInstance().getSettingInfo();

    map.putInt("threadMode", info.strategy);
    map.putBoolean("preSize", info.enablePresetSize);
    map.putBoolean("enableRenderNode", info.enableRenderNode);
    map.putBoolean("debugMenu", info.enableDebugMenu);

    return map;
  }

  private void startQRScanActivity() {
    Intent intent = new Intent(mContext, QRScanActivity.class);
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    mContext.startActivity(intent);
  }

  private void startFromUrl(String url) {
    TemplateDispatcher.dispatchUrl(mContext, url);
  }

  private void startFromUrlSingleTop(String url) {
    TemplateDispatcher.dispatchUrl(
        mContext, url, Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
  }

  private boolean isClosableExplorerActivity(@Nullable Activity activity) {
    return activity instanceof LynxViewShellActivity || activity instanceof QRScanActivity
        || activity instanceof LynxRecorderActivity;
  }

  private boolean isActivityUnavailable(@Nullable Activity activity) {
    return activity == null || activity.isFinishing()
        || (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1 && activity.isDestroyed());
  }

  private void executeOnMainThread(ThrowingCallable callable) {
    if (mHandler == null) {
      throw new IllegalStateException("LynxModuleAdapter is not initialized.");
    }
    if (Looper.myLooper() == Looper.getMainLooper()) {
      callable.call();
      return;
    }

    FutureTask<Void> task = new FutureTask<>(() -> callable.call());
    mHandler.post(task);
    try {
      task.get();
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new IllegalStateException("Interrupted while waiting for main-thread execution.", e);
    } catch (ExecutionException e) {
      Throwable cause = e.getCause();
      if (cause instanceof RuntimeException) {
        throw (RuntimeException) cause;
      }
      throw new IllegalStateException("Main-thread execution failed.", cause);
    }
  }

  private interface ThrowingCallable {
    Void call();
  }

  public void saveThemePreferences(String theme, String value) {
    SharedPreferences p =
        mContext.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE);
    p.edit().putString(theme, value).apply();
  }

  public void saveToLocalStorage(String key, String value) {
    if (key == null) {
      return;
    }
    SharedPreferences p =
        mContext.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE);
    p.edit().putString(key, value).apply();
  }

  @Nullable
  public String readFromLocalStorage(String key) {
    SharedPreferences p =
        mContext.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE);
    String value = p.getString(key, null);
    return value;
  }

  private LynxModuleAdapter() {}
}
