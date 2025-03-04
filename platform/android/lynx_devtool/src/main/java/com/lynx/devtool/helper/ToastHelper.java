// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Looper;

public class ToastHelper {
  private static class Alert implements Runnable {
    @Override
    public void run() {
      if (mContext != null) {
        showAlertSync(mTitle, mMessage, mContext);
      }
    }

    public void setData(String title, String message, Context context) {
      mTitle = title;
      mMessage = message;
      mContext = context;
    }

    private String mTitle;
    private String mMessage;
    private Context mContext;
  }

  public static void showAlert(String title, String message, Context context) {
    Handler mainHandler = new Handler(Looper.getMainLooper());
    Alert alert = new Alert();
    alert.setData(title, message, context);
    mainHandler.post(alert);
  }

  public static void showAlertSync(String title, String message, Context context) {
    try {
      AlertDialog.Builder builder = new AlertDialog.Builder(context);
      AlertDialog dialog;
      if (message != null) {
        dialog = builder.setTitle(title)
                     .setMessage(message)
                     .setNegativeButton("OK",
                         new DialogInterface.OnClickListener() {
                           @Override
                           public void onClick(DialogInterface dialog, int which) {}
                         })
                     .create();
      } else {
        dialog = builder.setTitle(title)
                     .setNegativeButton("OK",
                         new DialogInterface.OnClickListener() {
                           @Override
                           public void onClick(DialogInterface dialog, int which) {}
                         })
                     .create();
      }
      dialog.show();
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }
}
