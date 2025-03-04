// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.app.Activity;
import android.content.Context;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.tasm.base.LLog;
import java.util.WeakHashMap;

public class LynxLogBoxOwner {
  private static final String TAG = "LynxLogBoxOwner";
  private WeakHashMap<Context, LynxLogBoxManager> mLogBoxManagers;

  public static LynxLogBoxOwner getInstance() {
    return LynxLogBoxOwner.SingletonHolder.INSTANCE;
  }

  private static class SingletonHolder {
    private static final LynxLogBoxOwner INSTANCE = new LynxLogBoxOwner();
  }

  private LynxLogBoxOwner() {
    mLogBoxManagers = new WeakHashMap<>();
  }

  public void dispatch(
      String msg, LogBoxLogLevel level, Context activity, LynxLogBoxProxy logBoxProxy) {
    LynxLogBoxManager manager = findManagerByActivity(activity);
    if (manager != null) {
      manager.onNewLog(msg, level, logBoxProxy);
    }
  }

  public void showConsoleLog(Context activity, LynxLogBoxProxy logBoxProxy) {
    LynxLogBoxManager manager = findManagerByActivity(activity);
    if (manager != null) {
      manager.showConsoleLog(logBoxProxy);
    }
  }

  public void onLoadTemplate(Context activity, LynxLogBoxProxy logBoxProxy) {
    LynxLogBoxManager manager = findManagerByActivityIfExist(activity);
    // if the template is loaded for the first time, manager must be null
    if (manager != null) {
      manager.onLynxViewReload(logBoxProxy);
    }
  }

  private LynxLogBoxManager findManagerByActivity(Context activity) {
    if (!(activity instanceof Activity)) {
      LLog.e(TAG, "param activity is null or not a Activity");
      return null;
    }
    if (((Activity) activity).isFinishing()) {
      LLog.i(TAG, "activity is finishing");
      return null;
    }
    LynxLogBoxManager manager = mLogBoxManagers.get(activity);
    if (manager == null) {
      LLog.i(TAG, "new activity");
      manager = new LynxLogBoxManager(activity);
      mLogBoxManagers.put(activity, manager);
    }
    return manager;
  }

  protected LynxLogBoxManager findManagerByActivityIfExist(Context activity) {
    if (activity == null) {
      LLog.e(TAG, "activity is null");
      return null;
    }
    return mLogBoxManagers.get(activity);
  }
}
