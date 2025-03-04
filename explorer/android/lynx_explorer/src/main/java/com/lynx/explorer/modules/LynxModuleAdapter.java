// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.devtoolwrapper.LynxDevtoolGlobalHelper;
import com.lynx.explorer.scan.QRScanActivity;
import com.lynx.explorer.shell.TemplateDispatcher;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.LynxEnv;

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

  void pageBack() {
    new Thread() {
      public void run() {
        try {
          Instrumentation inst = new Instrumentation();
          inst.sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);
        } catch (Exception e) {
          Log.e("Exception when onBack", e.toString());
        }
      }
    }.start();
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

  private LynxModuleAdapter() {}
}
