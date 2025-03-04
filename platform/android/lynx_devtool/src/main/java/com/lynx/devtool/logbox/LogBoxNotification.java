// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.app.Dialog;
import android.content.Context;
import android.graphics.drawable.GradientDrawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.example.lynxdevtool.R;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

class LogBoxNotification {
  private static final String TAG = "LogBoxNotification";
  private static final int MAX_COUNT_NUMBER_LENGTH = 2;
  private final WeakReference<Context> mContext;
  private final WeakReference<LynxLogBoxManager> mManager;
  private final Map<LogBoxLogLevel, NotificationViewHolder> mNotificationMap;
  private final Map<LogBoxLogLevel, Boolean> mIsLevelDirty;
  private Dialog mDialog = null;
  private LinearLayout mDialogContentView = null;

  private class NotificationViewHolder {
    private TextView mBriefLogInfoView;
    private TextView mLogCountView;
    private View mNotificationView;
    private LogBoxLogLevel mLevel;

    public NotificationViewHolder(Context context, ViewGroup rootView, LogBoxLogLevel level) {
      mNotificationView = LayoutInflater.from(context).inflate(
          R.layout.devtool_logbox_notification, rootView, false);
      mBriefLogInfoView = mNotificationView.findViewById(R.id.notification_text_brief_log);
      mLogCountView = mNotificationView.findViewById(R.id.notification_text_log_count);
      mLevel = level;

      View cancelButton = mNotificationView.findViewById(R.id.notification_button_cancel);
      cancelButton.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          handleClose(mLevel);
        }
      });

      mBriefLogInfoView.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          handleClick(mLevel);
        }
      });
      mBriefLogInfoView.setSingleLine();

      ImageView logCountCircle =
          mNotificationView.findViewById(R.id.notification_view_log_count_circle);
      GradientDrawable circle = (GradientDrawable) logCountCircle.getDrawable();
      if (mLevel == LogBoxLogLevel.Error) {
        circle.setColor(context.getResources().getColor(R.color.logbox_red));
      } else if (mLevel == LogBoxLogLevel.Warn) {
        circle.setColor(context.getResources().getColor(R.color.logbox_yellow));
      }
    }

    public void setBriefInfo(final String msg) {
      mBriefLogInfoView.setText(msg);
    }

    public void setLogCount(final int count) {
      String countStr = String.valueOf(count);
      if (countStr.length() > MAX_COUNT_NUMBER_LENGTH) {
        countStr = countStr.charAt(0) + "...";
      }
      mLogCountView.setText(countStr);
    }

    public View getView() {
      return mNotificationView;
    }
  }

  protected LogBoxNotification(Context context, LynxLogBoxManager manager) {
    mContext = new WeakReference<>(context);
    mManager = new WeakReference<>(manager);
    mNotificationMap = new HashMap<>();
    mIsLevelDirty = new HashMap<>();
  }

  private void initDialog(Context context) {
    mDialog = new Dialog(context);
    mDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
    mDialogContentView = new LinearLayout(context);
    mDialogContentView.setOrientation(LinearLayout.VERTICAL);

    mDialog.setContentView(mDialogContentView);
    mDialog.setCancelable(false);
    if (mDialog.getWindow() != null) {
      mDialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
      mDialog.getWindow().setDimAmount(0);
      mDialog.getWindow().setLayout(
          WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.WRAP_CONTENT);
      mDialog.getWindow().setGravity(Gravity.BOTTOM);
      // cancel dismiss when touch out side
      mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL);
      // cancel focus
      mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
      // layer below keyboard
      mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
      // cancel floating
      mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN);
    }
  }

  public void updateInfo(LogBoxLogLevel level, String msg, int logCount) {
    mIsLevelDirty.put(level, false);
    Context context = mContext.get();
    if (context == null) {
      return;
    }
    if (logCount <= 0) {
      hideNotification(level);
      return;
    }
    if (mDialog == null) {
      initDialog(context);
    }
    NotificationViewHolder viewHolder = mNotificationMap.get(level);
    if (viewHolder == null) {
      viewHolder = new NotificationViewHolder(context, mDialogContentView, level);
      mNotificationMap.put(level, viewHolder);
    }

    viewHolder.setBriefInfo(msg);
    viewHolder.setLogCount(logCount);
    View notificationView = viewHolder.getView();
    if (mDialogContentView != notificationView.getParent()) {
      mDialogContentView.addView(notificationView);
      if (!mDialog.isShowing()) {
        mDialog.show();
      }
    }
  }

  private void handleClick(LogBoxLogLevel level) {
    LynxLogBoxManager manager = mManager.get();
    if (manager != null) {
      manager.onNotificationClick(level);
    }
  }

  private void handleClose(LogBoxLogLevel level) {
    LynxLogBoxManager manager = mManager.get();
    if (manager != null) {
      manager.onNotificationClose(level);
    }
    hideNotification(level);
  }

  private void hideNotification(LogBoxLogLevel level) {
    if (mDialog == null) {
      return;
    }
    NotificationViewHolder notification = mNotificationMap.get(level);
    if (notification != null) {
      mDialogContentView.removeView(notification.getView());
    }
    if (mDialogContentView.getChildCount() == 0 && mDialog.isShowing()) {
      mDialog.dismiss();
    }
  }

  public void invalidate(LogBoxLogLevel level) {
    mIsLevelDirty.put(level, true);
  }

  public boolean isDirty(LogBoxLogLevel level) {
    Boolean dirty = mIsLevelDirty.get(level);
    return dirty == null ? true : dirty;
  }

  public void destroy() {
    if (mDialog != null) {
      mDialog.dismiss();
      mDialog = null;
    }
  }
}
