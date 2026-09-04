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
import com.lynx.explorer.routing.RequestedRuntime;
import com.lynx.explorer.routing.RouteCoordinator;
import com.lynx.explorer.routing.RouteResult;
import com.lynx.explorer.routing.RouteSource;
import com.lynx.explorer.scan.QRScanActivity;
import com.lynx.react.bridge.Callback;
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
    openRoute(context, url, "automatic", null);
  }

  public void openRoute(Context context, String url, String runtime, @Nullable Callback callback) {
    ActivityRequest request = new ActivityRequest(context);
    RequestedRuntime requested = "sparkling".equalsIgnoreCase(runtime) ? RequestedRuntime.SPARKLING
        : ("lynx".equalsIgnoreCase(runtime) || "legacy".equalsIgnoreCase(runtime))
        ? RequestedRuntime.LYNX
        : RequestedRuntime.AUTOMATIC;
    mHandler.post(() -> {
      RouteResult result = RouteCoordinator.open(
          getStartContext(request.getActivity()), url, requested, RouteSource.NATIVE_MODULE);
      if (callback != null) {
        JavaOnlyMap response = new JavaOnlyMap();
        response.putBoolean("success", result.getAccepted());
        response.putBoolean("accepted", result.getAccepted());
        response.putString("code", result.getCode());
        response.putString("msg", result.getMessage());
        response.putString("message", result.getMessage());
        callback.invoke(response);
      }
    });
  }

  public void navigateBack(Context context, @Nullable Callback callback) {
    ActivityRequest request = new ActivityRequest(context);
    mHandler.post(() -> {
      Activity activity = request.getActivity();
      RouteResult result = activity == null
          ? RouteResult.failure("route_owner_unavailable", "No owning Activity is available.")
          : RouteCoordinator.navigateBack(activity);
      JavaOnlyMap response = new JavaOnlyMap();
      response.putBoolean("success", result.getAccepted());
      response.putBoolean("accepted", result.getAccepted());
      response.putString("code", result.getCode());
      response.putString("msg", result.getMessage());
      response.putString("message", result.getMessage());
      if (callback != null) {
        callback.invoke(response);
      }
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

  private void startQRScanActivity(@Nullable Activity activity) {
    Context startContext = getStartContext(activity);
    Intent intent = new Intent(startContext, QRScanActivity.class);
    if (!(startContext instanceof Activity)) {
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    }
    startContext.startActivity(intent);
  }

  private void startFromUrlSingleTop(String url) {
    mHandler.post(()
                      -> RouteCoordinator.open(
                          mContext, url, RequestedRuntime.AUTOMATIC, RouteSource.DEVTOOL));
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
