// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import android.content.Context;
import android.graphics.Point;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import com.example.lynxdevtool.R;
import java.util.ArrayList;

public class LynxRecorderReplayStateView extends LinearLayout {
  public static final int DOWNLOAD_JSON_FILE = 0;
  public static final int PARSING_JSON_FILE = 1;
  public static final int HANDLE_ACTION_LIST = 2;
  public static final int INVALID_JSON_FILE = 3;
  public static final int RECORD_ERROR_MISS_TEMPLATEJS = 4;
  public static final int ERROR_DOWNLOAD_FAILED = 5;
  public static final int ERROR_MISS_LYNXRECORDER_HEADER = 6;

  private static final ArrayList<String> mState = new ArrayList<String>();

  static {
    mState.add(DOWNLOAD_JSON_FILE, "Download json file");
    mState.add(PARSING_JSON_FILE, "Parsing json file");
    mState.add(HANDLE_ACTION_LIST, "Handle action list");
    mState.add(INVALID_JSON_FILE, "Invalid Json File");
    mState.add(RECORD_ERROR_MISS_TEMPLATEJS, "Record Error: Miss template.js");
    mState.add(ERROR_DOWNLOAD_FAILED, "LynxRecorder artifact download failed");
    mState.add(ERROR_MISS_LYNXRECORDER_HEADER,
        "Miss LynxRecorder header : file://lynxrecorder?url={{{url}}}");
  }

  private TextView mText;
  private ProgressBar mProgress;

  public LynxRecorderReplayStateView(Context context) {
    super(context);
    setId(R.id.recorder_state_view);
    init(context);
  }

  private void init(Context context) {
    View view = LayoutInflater.from(context).inflate(R.layout.recorder_replay_state, this);
    updateScreenMetrics();
    this.setGravity(Gravity.CENTER);
    mText = findViewById(R.id.textView2);
    mProgress = findViewById(R.id.progressBar2);
  }

  public void updateScreenMetrics() {
    WindowManager wm = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
    Point size = new Point();
    wm.getDefaultDisplay().getSize(size);
    ViewGroup.LayoutParams layout_params = this.getLayoutParams();
    if (layout_params == null) {
      layout_params = new LayoutParams(size.x, size.y);
    } else {
      layout_params.width = size.x;
      layout_params.height = size.y;
    }
    this.setLayoutParams(layout_params);
  }

  private int getProgress(int stateCode) {
    return (stateCode + 1) * 100 / (mState.size() + 1);
  }

  public void setReplayState(int stateCode) {
    if (mText != null && mProgress != null) {
      if (stateCode >= 0 && stateCode < mState.size()) {
        mText.setText(mState.get(stateCode));
        mProgress.setProgress(getProgress(stateCode));
      } else {
        mText.setText("Unknown State");
        mProgress.setProgress(0);
      }
    }
  }
}
