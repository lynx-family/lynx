// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.app.Fragment;
import android.content.Context;
import com.lynx.tasm.base.LLog;

// A empty fragment be used to listen the onDestory event of activity.
// When the activity is destroying, release resources like dialogs and views
// that hold the activity in LynxLogBoxManager to avoid memory leak.
public class LogBoxLifeCycleListenerFragment extends Fragment {
  private static final String TAG = "LogBoxLifeCycleListenerFragment";

  @Override
  public void onDestroy() {
    LLog.i(TAG, "on destroy");
    Context activity = getActivity();
    LynxLogBoxManager manager =
        LynxLogBoxOwner.getInstance().findManagerByActivityIfExist(activity);
    if (manager != null) {
      manager.destroy();
    }
    super.onDestroy();
  }
}
