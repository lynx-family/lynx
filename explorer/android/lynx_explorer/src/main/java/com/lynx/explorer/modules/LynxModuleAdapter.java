// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import androidx.annotation.Nullable;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.devtoolwrapper.LynxDevtoolGlobalHelper;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.explorer.scan.QRScanActivity;
import com.lynx.explorer.shell.TemplateDispatcher;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.utils.ContextUtils;
import java.lang.ref.WeakReference;

public class LynxModuleAdapter {
  private Context mContext;
  private Handler mHandler;

  private LynxDevtoolCardListener mListener = new LynxDevtoolCardListener() {
    @Override
    public void open(String url) {
      startFromUrlSingleTop(url);
    }
  };

  private static final int OPEN_SCHEMA = 0;
  private static final int OPEN_SCAN = 1;
  private static final LynxModuleAdapter sInstance = new LynxModuleAdapter();

  private static class ActivityRequest {
    @Nullable private final WeakReference<Activity> mActivityRef;

    ActivityRequest(Context context) {
      Activity activity = context != null ? ContextUtils.getActivity(context) : null;
      mActivityRef = activity != null ? new WeakReference<>(activity) : null;
    }

    @Nullable
    Activity getActivity() {
      Activity activity = mActivityRef != null ? mActivityRef.get() : null;
      if (activity == null || activity.isFinishing() || activity.isDestroyed()) {
        return null;
      }
      return activity;
    }
  }

  private static class OpenSchemaRequest extends ActivityRequest {
    private final String mUrl;

    OpenSchemaRequest(Context context, String url) {
      super(context);
      mUrl = url;
    }
  }

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
            startQRScanActivity(((ActivityRequest) msg.obj).getActivity());
            break;
          case OPEN_SCHEMA:
            OpenSchemaRequest request = (OpenSchemaRequest) msg.obj;
            startFromUrl(request.getActivity(), request.mUrl);
            break;
          default:
        }
      }
    };
    LynxEnv.inst().registerModule("ExplorerModule", ExplorerModule.class);
    LynxEnv.inst().registerModule("LynxNodeAPI", LynxNodeAPIModule.class);

    LynxDevtoolGlobalHelper.getInstance().registerCardListener(mListener);
  }

  public void openScan() {
    openScan(mContext);
  }

  public void openScan(Context context) {
    Message msg = Message.obtain();
    msg.obj = new ActivityRequest(context);
    msg.what = OPEN_SCAN;
    mHandler.sendMessage(msg);
  }

  public void openSchema(String url) {
    openSchema(mContext, url);
  }

  public void openSchema(Context context, String url) {
    Message msg = Message.obtain();
    msg.obj = new OpenSchemaRequest(context, url);
    msg.what = OPEN_SCHEMA;
    mHandler.sendMessage(msg);
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

  private void startQRScanActivity(@Nullable Activity activity) {
    Context startContext = getStartContext(activity);
    Intent intent = new Intent(startContext, QRScanActivity.class);
    if (!(startContext instanceof Activity)) {
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    }
    startContext.startActivity(intent);
  }

  private void startFromUrl(@Nullable Activity activity, String url) {
    Context startContext = getStartContext(activity);
    int flags = startContext instanceof Activity ? 0 : Intent.FLAG_ACTIVITY_NEW_TASK;
    TemplateDispatcher.dispatchUrl(startContext, url, flags);
  }

  private void startFromUrlSingleTop(String url) {
    TemplateDispatcher.dispatchUrl(
        mContext, url, Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
  }

  private Context getStartContext(@Nullable Activity activity) {
    return activity != null ? activity : mContext;
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
